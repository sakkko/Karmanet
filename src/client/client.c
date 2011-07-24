#include "client.h"

void add_fd(int fd, fd_set *fdset) {
	FD_SET(fd, fdset);
	if (fd > maxd) {
		maxd = fd;
	}	
}

void print_addr_list(struct sockaddr_in *addr, int dim) {
	int i = 0;
	int off = sizeof(struct sockaddr_in);
	for (i = 0; i < dim; i ++) {
		printf("%d address: %lu : %u\n", i, (unsigned long)addr[off * i].sin_addr.s_addr, addr[off * i].sin_port);
	}		

}

/*
* Calcola indice prestazione peer: peer_rate = uptime + totalram
*/
void set_rate() {
	if ((peer_rate = get_peer_rate()) < 0) {
		perror("errore sysinfo");
		peer_rate = 0;
	}

	printf("MY-RATE = %ld\n\n", peer_rate);
}


struct sockaddr_in *str_to_addr(const char *str, int dim) {
	struct sockaddr_in *ret = (struct sockaddr_in *)malloc(dim * sizeof(struct sockaddr_in));
	int offset = sizeof(long) + sizeof(short);
	int i;
	
	for (i = 0; i < dim; i  ++) {
		str2addr(&ret[i], str + (i * offset));
	}
	
	return ret;
}

/*
* Funzione di inizializzazione generale.
*/
int init(fd_set *fdset) {
	if ((udp_sock = udp_socket()) < 0) {
		perror("errore in socket");
		return -1;
	}
	
	maxd = -1;
	have_child = 0;
	curr_child_redirect = 0;
	curr_p_count = 0;
	is_sp = 0;
	free_sock = 0;
	
	FD_ZERO(fdset);
	add_fd(fileno(stdin), fdset);
	add_fd(udp_sock, fdset);

	set_rate();
	init_index();
	retx_init();
	
	return 0;
}

/*
 * Funzione che gestisce l'intero processo di connessione alla rete.
 * Ritorna 0 in caso di successo e -1 in caso di errore.
 * Quando la funzione torna senza errore il client ha effettuato il join
 * ed è entrato completamente nella rete P2P.
 */
int start_process(fd_set *fdset, struct sockaddr_in *sp_addr) {
	struct sockaddr_in *addr_list;
	int addr_list_len, rc, error;

	//ottengo la lista dei superpeer dal bootstrap
	addr_list = get_sp_list(&addr_list_len, &error);
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
		if ((rc = connect_to_sp(sp_addr, addr_list, addr_list_len)) < 0) {
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
 * Funzione che inizializza la socket di ascolto tcp.
 * Ritorna 0 in caso successo e -1 in caso di errore.
 */
 
int set_listen_socket(fd_set *fdset) {
	struct sockaddr_in my_addr;

	if ((tcp_listen = tcp_socket()) < 0) {
		perror("set_listen_socket error - can't initialize tcp socket");
		return -1;
	}
	
	set_addr_any(&my_addr, TCP_PORT);
	if (inet_bind(tcp_listen, &my_addr)){
		perror ("set_listen_socket error - bind failed on tcp socket");
		return -1;
	}
	
	if (listen(tcp_listen, BACKLOG) < 0) {
		perror ("set_listen_socket error - listen failed on tcp socket");
		return -1;
	}

	add_fd(tcp_listen, fdset);

	return 0;
}

/*
 * Funzione prova a connetersi ai superpeer indicati nella lista passata come parametro.
 * Ritorna 0 in caso di successo e -1 se si è verificato un errore o se non è riuscita
 * a connettersi a nessun superpeer della lista.
 */
int join_overlay(fd_set *fdset, const struct sockaddr_in *sp_addr_list, int list_len) {
	int i, ok = 1;

	for (i = 0;  i < list_len; i ++) {
		if (ok) {
			if ((tcp_sock[free_sock] = tcp_socket()) < 0) {
				perror("join_overlay error - can't initialize tcp socket");
				return -1;
			}
		}
		if (tcp_connect(tcp_sock[free_sock], &sp_addr_list[i]) < 0){
			perror("join_overlay error - can't connect to superpeer");
			ok = 0;
			continue; //provo il prossimo indirizzo
		} else {
			printf("Connected with superpeer %s:%d\n", inet_ntoa(sp_addr_list[i].sin_addr), ntohs(sp_addr_list[i].sin_port));
			ok = 1;
			add_fd(tcp_sock[free_sock], fdset);
			free_sock ++;
		}
	}

	if (!ok) {
		close(tcp_sock[free_sock]);
	}

	if (free_sock == 0) {
		return -1;
	}

	return 0;
	
}

/*
 * Funzione di inizializzazione del superpeer.
 */
int init_superpeer(fd_set *fdset, const struct sockaddr_in *sp_addr_list, int list_len) {
	printf("INIT SUPER PEER\n");
	//inizializzo socket di ascolto
	if (set_listen_socket(fdset) < 0) {
		fprintf(stderr, "init_superpeer error - set_listen_socket failed\n");
		return -1;
	}

	if (sp_addr_list != NULL) {
		//provo a connettermi agli altri superpeer
		if (join_overlay(fdset, sp_addr_list, list_len) < 0) {
			fprintf(stderr, "init_superpeer error - join_overlay failed\n");
			return -1;
		}
	}

	is_sp = 1;

	return 0;
	
}

/*
 * Funzione che richiede al server di bootstrap la lista di indirizzi dei superpeer 
 * presenti nella rete e setta la veriabile error in caso di errori.
 * Ritorna la lista degli indirizzi dei superpeer oppure NULL se il peer p stato
 * promosso superpeer dal bootstap. In caso di errore ritorna NULL e la variabile
 * error è impostata a 1.
 */
struct sockaddr_in *get_sp_list(int *len, int *error) {
	struct packet send_pck, recv_pck;

	new_join_packet(&send_pck, get_index()); 
	if (retx_send(udp_sock, &bs_addr, &send_pck) < 0) {
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
int connect_to_sp(struct sockaddr_in *sp_addr, const struct sockaddr_in *addr_list, int addr_list_len) {
	int i, ret;
	
	for (i = 0; i < addr_list_len; i ++) {
		if ((ret = call_sp(&addr_list[i])) < 0) {
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
int call_sp(const struct sockaddr_in *addr_to_call) {
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
					 return call_sp(&tmp_addr);
				 }
			} else if (!strncmp(recv_pck.cmd, CMD_PROMOTE, CMD_STR_LEN)) {
				return  1;
			} 
		 }
	}

	return -1;
}

int join_peer(const struct sockaddr_in *addr, const struct packet *pck, struct peer_list_ch_info *plchinfo) {
	unsigned long peer_rate;
	struct packet tmp_packet;
	struct sockaddr_in *best_peer_addr;
	int rc;

	if (is_sp == 0) {
		new_err_packet(&tmp_packet, pck->index);
	} else {
		if (curr_p_count < MAX_P_COUNT) {
			//posso accettare il peer
			peer_rate = btol(pck->data);
			if ((rc = pthread_mutex_lock(&plchinfo->thinfo.mutex)) != 0) {
				fprintf(stderr, "join_peer error - can't acquire lock: %s\n", strerror(rc));
				return -1;
			}
			sorted_insert_peer(addr, peer_rate);
			if ((rc = pthread_mutex_unlock(&plchinfo->thinfo.mutex)) != 0) {
				fprintf(stderr, "join_peer error - can't release lock: %s\n", strerror(rc));
				return -1;
			}
			new_ack_packet(&tmp_packet, pck->index);
			printf("aggiunto peer %s:%d\nrate: %ld\n", inet_ntoa(addr->sin_addr), addr->sin_port, peer_rate);
			printf("dimensione lista peer: %d\n", get_list_count(peer_list_head));
			insert_request(addr, pck->index);
			curr_p_count++;
		} else {
			printf("troppi P!!!\n");
			if (have_child) {
				new_redirect_packet(&tmp_packet, pck->index, &child_addr);
			} else {
				new_promote_packet(&tmp_packet, get_index());
				if ((rc = pthread_mutex_lock(&plchinfo->thinfo.mutex)) != 0) {
					fprintf(stderr, "join_peer error - can't acquire lock: %s\n", strerror(rc));
					return -1;
				}
				best_peer_addr = get_addr_head();
				if ((rc = pthread_mutex_unlock(&plchinfo->thinfo.mutex)) != 0) {
					fprintf(stderr, "join_peer error - can't release lock: %s\n", strerror(rc));
					return -1;
				}
				if (retx_send(udp_sock, get_addr_head(), &tmp_packet) < 0) {
					fprintf(stderr, "join_peer error: retx_send failed\n");
					return -1;
				} else {
					//TODO per essere sicuri che il peer è diventato superpeer dovremmo aspettare
					//un ack e poi settare l'indirizzo del figlio
					addrcpy(&child_addr, best_peer_addr);
					have_child = 1;
					return 0;
				}
			}
		}
	}

	if (mutex_send(udp_sock, addr, &tmp_packet) < 0) {
		fprintf(stderr, "join_peer error: mutex_send failed\n");
		return -1;
	}

	return 0;
}

void udp_handler(int udp_sock, fd_set *allset, struct sp_checker_info *spchinfo, struct peer_list_ch_info *plchinfo) {
	struct packet tmp_pck;
	struct sockaddr_in addr;
	int len = sizeof(struct sockaddr_in); 
	struct sockaddr_in *addr_list;
	int error;
	int list_len;
	
	
	if (recvfrom_packet(udp_sock, &addr, &tmp_pck, &len) < 0) {
		printf("ERRORE RECV_PACKET\n");
		return;
	}

	//TODO se ho già processato questa richiesta nel immediato passato rinvio l'ack
	
			
	if (!strncmp(tmp_pck.cmd, CMD_ACK, CMD_STR_LEN)) {
		//ricevuto ack
		retx_stop(tmp_pck.index);
	} 
	
	if (is_sp == 1) {
		//sono sp
		 if (!strncmp(tmp_pck.cmd, CMD_JOIN, CMD_STR_LEN)) {
			join_peer(&addr, &tmp_pck, plchinfo);
			insert_request(&addr,tmp_pck.index);
		} else if (!strncmp(tmp_pck.cmd, CMD_PING, CMD_STR_LEN)) { 
			//printf("ricevuto ping\n");
			//updateflag di addr
			printf("Ricevuto ping da %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
			update_peer_flag(plchinfo, &addr);
	 		new_pong_packet(&tmp_pck, tmp_pck.index);
		 	if (mutex_send(udp_sock, &addr, &tmp_pck) < 0) {
		 		perror ("errore in udp_send");
		 	}
		} else if (!strncmp(tmp_pck.cmd, CMD_LEAVE, CMD_STR_LEN)) { 
			//rimuovi addr
		} else if (!strncmp(tmp_pck.cmd, CMD_WHOHAS, CMD_STR_LEN)) { 
			;
		}
	} else {
		//sono solo peer
		if (!strncmp(tmp_pck.cmd, CMD_PONG, CMD_STR_LEN)) { 
			//printf("ricevuto pong\n");
			//updateflag di addr
			printf("Ricevuto pong da superpeer\n");
			update_sp_flag(spchinfo);
		}
		else if (!strncmp(tmp_pck.cmd, CMD_PROMOTE, CMD_STR_LEN)) { 
			printf("\n\n\nRICEVUTO PROMOTE\n\n\n");

			//updateflag di addr
	 		new_ack_packet(&tmp_pck, tmp_pck.index);
		 	if (mutex_send(udp_sock, &addr, &tmp_pck) < 0) {
		 		perror ("errore in udp_send");
		 	}
		 	printf("richiedo lista sp\n");
			addr_list = get_sp_list(&list_len, &error);
			if (error) {
				fprintf(stderr, "udp_handler - can't get superpeer address list from bootstrap\n");
			}
			
		 	printf("provo init sp\n");
			init_superpeer(allset, addr_list,list_len);
			
		 	printf("invio register\n");
		 	new_register_packet(&tmp_pck, get_index());
			if (retx_send(udp_sock, &bs_addr, &tmp_pck) < 0) {
				perror("errore in udp_send");
			}
		}
		
	}
}

/*
 * Funzione di terminazione del processo. Chiude la pipe e ferma i thread
 * di controllo e di ping.
 */
int end_process(fd_set *allset, struct sp_checker_info *spchinfo, struct peer_list_ch_info *plchinfo, struct pinger_info *pinfo) {
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

int main (int argc,char *argv[]){
//	fork();
//	fork();
	int i; //lunghezza indirizzi ricevuti
	int ready;
	fd_set rset, allset;

	struct sockaddr_in sp_addr;
	struct pinger_info ping_info;
	struct sp_checker_info *sp_check_info;
	struct peer_list_ch_info *pl_check_info;

	// controlla numero degli argomenti
	if (argc != 2) { 
		fprintf(stderr, "Utilizzo: %s <indirizzo IP server>\n", argv[0]);
		return 1;
	}
	
	if (set_addr_in(&bs_addr, argv[1], BS_PORT) < 0) {
		fprintf(stderr, "errore in inet_pton per %s", argv[1]);
		return 1;
	}	

	if (init(&allset) < 0) {
		fprintf(stderr, "Errore durante l'inizializzazione\n");
		return 1;
	}
	
	//provo a entrare nella rete
	if (start_process(&allset, &sp_addr) < 0) {
		fprintf(stderr, "Initialization error - abort\n");
		return 1;
	}

	//imposto la socket per il thread che fa i ping
	ping_info.socksd = udp_sock;

	if (is_sp) {
		//alloco la struttura per il controllo della lista dei peer
		pl_check_info = (struct peer_list_ch_info *)malloc(sizeof(struct peer_list_ch_info));
		//setto il puntatore alla variabile che contiene l'indirizzo della testa della lista
		pl_check_info->peer_list = &peer_list_head;
		//faccio partire il thread che controlla la lista
		if (peer_list_checker_run(pl_check_info) < 0) {
			fprintf(stderr, "Can't star peer list checker\n");
			return 1;
		}
		//imposto l'indirizzo a cui fare il ping
		ping_info.addr_to_ping = &bs_addr;
	} else {
		//alloco la struttura per il controllo del superpeer
		sp_check_info = (struct sp_checker_info *)malloc(sizeof(struct sp_checker_info));
		//imposto il flag di presenza del superpeer
		sp_check_info->sp_flag = 1;
		pipe(fd);
		//imposto la pipe
		sp_check_info->sp_checker_pipe = fd[1];
		//faccio partire il thread che controlla il flag di presenza del superpeer
		if (sp_checker_run(sp_check_info) < 0) {
			fprintf(stderr, "Can't start superpeer checker\n");
			return 1;
		}
		add_fd(fd[0], &allset);			
		//imposto l'indirizzo a cui fare il ping
		ping_info.addr_to_ping = &sp_addr;
	}

	//faccio partire il processo dei ping
	if (pinger_run(&ping_info) < 0) {
		fprintf(stderr, "Can't start ping thread\n");
		return 1;
	}

	//main loop - select	
	while (1) {
		rset = allset;
		if ((ready = select(maxd + 1, &rset, NULL, NULL, NULL)) < 0) {
      		perror("errore in select");
			exit(1);
		}
	    	
		if (FD_ISSET(udp_sock, &rset)) { 
			udp_handler (udp_sock , &allset, sp_check_info, pl_check_info);
		} else if (FD_ISSET(fileno(stdin), &rset)) {
			;
		} else if (FD_ISSET(tcp_listen, &rset)) {
			printf("descrittore listen attivo\n");
			if (free_sock < MAX_TCP_SOCKET) {
				if ((tcp_sock[free_sock] = accept(tcp_listen, NULL, NULL)) < 0) {
					perror("errore in accept ");
					return 1;
				}
				add_fd(tcp_sock[free_sock], &allset);
				free_sock ++;
			}
		} else if (FD_ISSET(fd[0], &rset)) {
			char str[4];
			read(fd[0], str, 3);
			printf("pipe: %s\n", str);
			if (!strcmp(str, "RST")) {
				end_process(&allset, sp_check_info, pl_check_info, &ping_info);
				pl_check_info = NULL;
				sp_check_info = NULL;

				start_process(&allset, &sp_addr);

				if (is_sp) {
					//alloco la struttura per il controllo della lista dei peer
					pl_check_info = (struct peer_list_ch_info *)malloc(sizeof(struct peer_list_ch_info));
					//setto il puntatore alla variabile che contiene l'indirizzo della testa della lista
					pl_check_info->peer_list = &peer_list_head;
					//faccio partire il thread che controlla la lista
					if (peer_list_checker_run(pl_check_info) < 0) {
						fprintf(stderr, "Can't star peer list checker\n");
						return 1;
					}
					//imposto l'indirizzo a cui fare il ping
					ping_info.addr_to_ping = &bs_addr;
				} else {
					//alloco la struttura per il controllo del superpeer
					sp_check_info = (struct sp_checker_info *)malloc(sizeof(struct sp_checker_info));
					//imposto il flag di presenza del superpeer
					sp_check_info->sp_flag = 1;
					pipe(fd);
					//imposto la pipe
					sp_check_info->sp_checker_pipe = fd[1];
					//faccio partire il thread che controlla il flag di presenza del superpeer
					if (sp_checker_run(sp_check_info) < 0) {
						fprintf(stderr, "Can't start superpeer checker\n");
						return 1;
					}
					add_fd(fd[0], &allset);			
					//imposto l'indirizzo a cui fare il ping
					ping_info.addr_to_ping = &sp_addr;
				}

				//faccio partire il processo dei ping
				if (pinger_run(&ping_info) < 0) {
					fprintf(stderr, "Can't start ping thread\n");
					return 1;
				}

			}

		} else {
			for (i = 0; i < free_sock; i ++) {
				if (FD_ISSET(tcp_sock[i], &rset)) {
					//TODO tcp_handler();
					if (--ready <= 0) {
						break;
					}
				}
			}
		}			
	}

	return 0;
}

