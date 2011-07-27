#include "client.h"
//funzione di prova mai utilizzata
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


/*
* Funzione di inizializzazione generale.
*/
int init() {
	if ((udp_sock = udp_socket()) < 0) {
		perror("errore in socket");
		return -1;
	}
	
	fd_init();	
	fd_add(fileno(stdin));
	fd_add(udp_sock);

	set_rate();
	init_index();
	retx_init();
	
	return 0;
}

int udp_handler(int udp_sock, const struct sockaddr_in *bs_addr) {
	struct packet tmp_pck;
	struct sockaddr_in addr;
	int len = sizeof(struct sockaddr_in); 
	
	
	if (recvfrom_packet(udp_sock, &addr, &tmp_pck, &len) < 0) {
		printf("ERRORE RECV_PACKET\n");
		return -1;
	}

	//TODO se ho già processato questa richiesta nel immediato passato rinvio l'ack
	
			
	if (!strncmp(tmp_pck.cmd, CMD_ACK, CMD_STR_LEN)) {
		//ricevuto ack
		retx_stop(tmp_pck.index);
	} 
	
	if (is_sp == 1) {
		//sono sp
		 if (!strncmp(tmp_pck.cmd, CMD_JOIN, CMD_STR_LEN)) {
			if (join_handler(udp_sock, &addr, &tmp_pck) < 0) {
				fprintf(stderr, "udp_handler error - join error\n");
				return -1;
			}
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
			printf("Ricevuto pong da superpeer\n");
			update_sp_flag(spchinfo);
		}
		else if (!strncmp(tmp_pck.cmd, CMD_PROMOTE, CMD_STR_LEN)) { 
			printf("\n\n\nRICEVUTO PROMOTE\n\n\n");

	 		new_ack_packet(&tmp_pck, tmp_pck.index);
		 	if (mutex_send(udp_sock, &addr, &tmp_pck) < 0) {
		 		perror ("errore in udp_send");
		 	}
			if (promote_handler(udp_sock, bs_addr) < 0) {
				fprintf(stderr, "udp_handler error - can't promote peer\n");
				return -1;
			}
		}
		
	}

	return 0;
}


int promote_handler(int udp_sock, const struct sockaddr_in *bs_addr) {
	struct sockaddr_in *addr_list;
	struct packet tmp_pck;
	int list_len, error = 0;

	
	printf("richiedo lista sp\n");
	addr_list = get_sp_list(udp_sock, bs_addr, &list_len, &error);
	if (error) {
		fprintf(stderr, "promote_handler error - can't get superpeer address list from bootstrap\n");
		return -1;
	}

	end_process();
	printf("provo init sp\n");
	if (init_superpeer(udp_sock, addr_list, list_len) < 0) {
		fprintf(stderr, "promote_handler error - can't initialize superpeer\n");
		free(addr_list);
		return -1;
	}
	free(addr_list);

	run_threads(bs_addr, NULL);
	printf("invio register\n");
	new_register_packet(&tmp_pck, get_index());
	if (retx_send(udp_sock, bs_addr, &tmp_pck) < 0) {
		perror("errore in udp_send");
		return -1;
	}

	pinfo.addr_to_ping = bs_addr;

	if (pinger_run(&pinfo) < 0) {
		fprintf(stderr, "promote_handler error - can't start pinger\n");
		return -1;

	}
	return 0;
}

int join_handler(int udp_sock, const struct sockaddr_in *addr, const struct packet *pck) {
	struct packet tmp_packet;

	if (is_sp == 0) {
		new_err_packet(&tmp_packet, pck->index);
	} else {
		if (curr_p_count < MAX_P_COUNT) {
			//posso accettare il peer
			if (join_peer(addr, btol(pck->data), plchinfo) < 0) {
				fprintf(stderr, "join_handler error - can't join peer\n");
				return -1;
			}
			new_ack_packet(&tmp_packet, pck->index);
			insert_request(addr, pck->index);
		} else {
			printf("troppi P!!!\n");
			if (have_child == 0) {
				if (promote_peer(udp_sock, plchinfo) < 0) {
					fprintf(stderr, "join_handler error - can't promote peer\n");
					return -1;
				}
			}
			new_redirect_packet(&tmp_packet, pck->index, &child_addr);
		}
	}

	if (mutex_send(udp_sock, addr, &tmp_packet) < 0) {
		fprintf(stderr, "join_handler error - mutex_send failed\n");
		return -1;
	}

	return 0;
}

/*
 * Funzione di terminazione del processo. Chiude la pipe e ferma i thread
 * di controllo e di ping.
 */
int end_process() {
	close(fd[0]);
	close(fd[1]);
	fd_remove(fd[0]);

	return stop_threads();

}

int run_threads(const struct sockaddr_in *bs_addr, const struct sockaddr_in *sp_addr) {
	pinfo.socksd = udp_sock;

	if (is_sp) {
		//alloco la struttura per il controllo della lista dei peer
		plchinfo = (struct peer_list_ch_info *)malloc(sizeof(struct peer_list_ch_info));
		//setto il puntatore alla variabile che contiene l'indirizzo della testa della lista
		plchinfo->peer_list = &peer_list_head;
		//faccio partire il thread che controlla la lista
		if (peer_list_checker_run(plchinfo) < 0) {
			fprintf(stderr, "Can't star peer list checker\n");
			return 1;
		}
		//imposto l'indirizzo a cui fare il ping
		pinfo.addr_to_ping = bs_addr;
	} else {
		//alloco la struttura per il controllo del superpeer
		spchinfo = (struct sp_checker_info *)malloc(sizeof(struct sp_checker_info));
		//imposto il flag di presenza del superpeer
		spchinfo->sp_flag = 1;
		pipe(fd);
		//imposto la pipe
		spchinfo->sp_checker_pipe = fd[1];
		//faccio partire il thread che controlla il flag di presenza del superpeer
		if (sp_checker_run(spchinfo) < 0) {
			fprintf(stderr, "Can't start superpeer checker\n");
			return 1;
		}
		fd_add(fd[0]);			
		//imposto l'indirizzo a cui fare il ping
		pinfo.addr_to_ping = sp_addr;
	}

	//faccio partire il processo dei ping
	if (pinger_run(&pinfo) < 0) {
		fprintf(stderr, "Can't start ping thread\n");
		return 1;
	}

	return 0;
	
}

int stop_threads() {
	//interrompo il thread che fa i ping
	if (pinger_stop(&pinfo) < 0) {
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
		plchinfo = NULL;
	} else {
		//interrompo il thread che controlla il superpeer
		if (sp_checker_stop(spchinfo) < 0) {
			fprintf(stderr, "end_process error - can't stop superpeer checker\n");
			return -1;
		}
		free(spchinfo);
		spchinfo = NULL;
	}
	
	return 0;
	
}

int main (int argc,char *argv[]){
	int i; //lunghezza indirizzi ricevuti
	int ready;

	struct sockaddr_in sp_addr;
	struct sockaddr_in bs_addr;

	// controlla numero degli argomenti
	if (argc != 2) { 
		fprintf(stderr, "Utilizzo: %s <indirizzo IP server>\n", argv[0]);
		return 1;
	}
	
	if (set_addr_in(&bs_addr, argv[1], BS_PORT) < 0) {
		fprintf(stderr, "errore in inet_pton per %s", argv[1]);
		return 1;
	}	

	if (init() < 0) {
		fprintf(stderr, "Errore durante l'inizializzazione\n");
		return 1;
	}
	
	//provo a entrare nella rete
	if (start_process(udp_sock, &sp_addr, &bs_addr, peer_rate) < 0) {
		fprintf(stderr, "Initialization error - abort\n");
		return 1;
	}

	if (run_threads(&bs_addr, &sp_addr) < 0) {
		fprintf(stderr, "Initialization error -can't run threads\n");
		return 1;
	}

	//main loop - select	
	while (1) {
		if ((ready = fd_select()) < 0) {
      		perror("errore in select");
			exit(1);
		}
	    	
		if (fd_ready(udp_sock)) { 
			udp_handler(udp_sock, &bs_addr);
		} else if (fd_ready(fileno(stdin))) {
			;
		} else if (fd_ready(tcp_listen)) {
			printf("descrittore listen attivo\n");
			if (free_sock < MAX_TCP_SOCKET) {
				if ((tcp_sock[free_sock] = accept(tcp_listen, NULL, NULL)) < 0) {
					perror("errore in accept ");
					return 1;
				}
				fd_add(tcp_sock[free_sock]);
				free_sock ++;
			}
		} else if (fd_ready(fd[0])) {
			char str[4];
			read(fd[0], str, 3);
			printf("pipe: %s\n", str);
			if (!strcmp(str, "RST")) {
				end_process();

				start_process(udp_sock, &sp_addr, &bs_addr, peer_rate);

				if (run_threads(&bs_addr, &sp_addr) < 0) {
					fprintf(stderr, "Initialization error -can't run threads\n");
					return 1;
				}

			}

		} else {
			for (i = 0; i < free_sock; i ++) {
				if (fd_ready(tcp_sock[i])) {
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

