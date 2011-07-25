#include "boot.h"

/*
 * Funzione che richiede al server di bootstrap la lista di indirizzi dei superpeer 
 * presenti nella rete e setta la veriabile error in caso di errori.
 * Ritorna la lista degli indirizzi dei superpeer oppure NULL se il peer p stato
 * promosso superpeer dal bootstap. In caso di errore ritorna NULL e la variabile
 * error è impostata a 1.
 */
struct sockaddr_in *get_sp_list(int udp_sock, const struct sockaddr_in *bs_addr, int *len, int *error) {
	struct packet send_pck, recv_pck;

	new_join_packet(&send_pck, get_index()); 
	if (retx_send(udp_sock, bs_addr, &send_pck) < 0) {
		perror("errore in sendto");
		*error = 1;
		return NULL;	
	}
	
	printf("attendo risposta\n");
	if (retx_recv(udp_sock, &recv_pck) < 0) {
		perror("errore in recvfrom");
		*error = 1;
		return NULL;	
	}

	printf("ricevuti dati cmd:%s index:%d data_len:%d data:%s\n", recv_pck.cmd, recv_pck.index, recv_pck.data_len, recv_pck.data);
	if (!strncmp(recv_pck.cmd, CMD_PROMOTE, CMD_STR_LEN)) {
		is_sp = 1;
		*error = 0;
		return NULL;
	} else if (!strncmp(recv_pck.cmd, CMD_LIST, CMD_STR_LEN)) {
		*len = recv_pck.data_len / 6;
		*error = 0;
		return str_to_addr(recv_pck.data, *len);
	} else {
		fprintf(stderr, "comando non riconosciuto\n");
		*error = 1;
		return NULL;
	}
}

/*
 * Funzione che prova a connettersi ad un superpeer tra quelli contenuti nella lista passata
 * come parametro e setta l'indirizzo del superpeer a cui si è connesso.
 * Ritorna 0 in caso di successo, 1 se il peer è stato promosso superpeer e -1 in caso di errore.
 */
int connect_to_sp(int udp_sock, struct sockaddr_in *sp_addr, const struct sockaddr_in *addr_list, int addr_list_len, unsigned long peer_rate) {
	int i, ret;
	
	for (i = 0; i < addr_list_len; i ++) {
		if ((ret = call_sp(udp_sock, &addr_list[i], peer_rate)) < 0) {
			fprintf(stderr, "get_sp_addr error: call_sp failed\n");
			continue;
		} else {
			break;
		}
	}
	
	if (ret == 0) {
		addrcpy(sp_addr, &addr_list[i]);
	}

	return ret;
}

/*
 * Funzione che prova a connettersi al superpeer il cui indirizzo è passato per parametro. 
 * Se il superpeer risponde con un redirect la funzione richiama se stessa con l'indirizzo
 * indicato nel campo dati del pacchetto redirect.
 * Ritorna -1 in caso di errore, 0 se il peer si è connesso al superpeer, 
 * 1 se il peer è stato promosso superpeer (ha ricevuto promote dal superpeer).
 */
int call_sp(int udp_sock, const struct sockaddr_in *addr_to_call, unsigned long peer_rate) {
	struct packet send_pck, recv_pck;
	struct sockaddr_in tmp_addr;

	new_join_packet_rate(&send_pck, get_index(), peer_rate);

	if (retx_send(udp_sock, addr_to_call, &send_pck) < 0) {
		fprintf(stderr, "call_sp error: retx_send failed\n");
	} else {
		if (retx_recv(udp_sock, &recv_pck) < 0) {
			fprintf(stderr, "get_sp_addr error: retx_recv failed\n");
		 } else {
			 if (!strncmp(recv_pck.cmd, CMD_ACK, CMD_STR_LEN)) {
				 return 0;
			 } else if (!strncmp(recv_pck.cmd, CMD_REDIRECT, CMD_STR_LEN)) {
				 if (recv_pck.data_len != 0) {
					 str2addr(&tmp_addr, recv_pck.data);
					 return call_sp(udp_sock, &tmp_addr, peer_rate);
				 }
			} else if (!strncmp(recv_pck.cmd, CMD_PROMOTE, CMD_STR_LEN)) {
				return  1;
			} 
		 }
	}

	return -1;
}

/*
 * Funzione che gestisce l'intero processo di connessione alla rete.
 * Ritorna 0 in caso di successo e -1 in caso di errore.
 * Quando la funzione torna senza errore il client ha effettuato il join
 * ed è entrato completamente nella rete P2P.
 */
int start_process(int udp_sock, fd_set *fdset, struct sockaddr_in *sp_addr, const struct sockaddr_in *bs_addr, unsigned long peer_rate) {
	struct sockaddr_in *addr_list;
	int addr_list_len, rc, error;

	//ottengo la lista dei superpeer dal bootstrap
	addr_list = get_sp_list(udp_sock, bs_addr, &addr_list_len, &error);
	if (error) {
		fprintf(stderr, "start_process error - can't get superpeer address list from bootstrap\n");
		return -1;
	}


	if (addr_list == NULL) {
		//sono superpeer
		if (init_superpeer(fdset, addr_list, addr_list_len) < 0) {
			fprintf(stderr, "start_process error - init_superpeer failed\n");
			return -1;
		}
	} else {
		//provo a connettermi a un superpeer della lista
		if ((rc = connect_to_sp(udp_sock, sp_addr, addr_list, addr_list_len, peer_rate)) < 0) {
			fprintf(stderr, "start_process error: can't connect to superpeer\n");
			return -1;
		}

		if (rc == 1) {
			//sono superpeer - ho ricevuto promote dal superpeer
			if (init_superpeer(fdset, addr_list, addr_list_len) < 0) {
				fprintf(stderr, "start_process error - init_superpeer failed\n");
				return -1;
			}
		} else {
			//sono peer
			printf("Sono peer, il mio superpeer è %s:%d\n", inet_ntoa(sp_addr->sin_addr), ntohs(sp_addr->sin_port));
		}

	}

 
	return 0;
}

/*
 * Funzione di terminazione del processo. Chiude la pipe e ferma i thread
 * di controllo e di ping.
 */
int end_process(fd_set *allset, int *fd, struct sp_checker_info *spchinfo, struct peer_list_ch_info *plchinfo, struct pinger_info *pinfo) {
	close(fd[0]);
	close(fd[1]);
	FD_CLR(fd[0], allset);

	//interrompo il thread che fa i ping
	if (pinger_stop(pinfo) < 0) {
		fprintf(stderr, "end_process error - can't stop pinger\n");
		return -1;
	}

	if (is_sp) {
		//interrompo il thread che controlla la lista dei peer
		if (peer_list_checker_stop(plchinfo) < 0) {
			fprintf(stderr, "end_process error - can't stop peer list checker\n");
			return -1;
		}
		free(plchinfo);
	} else {
		//interrompo il thread che controlla il superpeer
		if (sp_checker_stop(spchinfo) < 0) {
			fprintf(stderr, "end_process error - can't stop superpeer checker\n");
			return -1;
		}
		free(spchinfo);
	}
	
	return 0;
}

