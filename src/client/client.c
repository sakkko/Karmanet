#include "client.h"

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
int init(int *udp_sock, fd_set *fdset) {
	if ((*udp_sock = udp_socket()) < 0) {
		perror("errore in socket");
		return -1;
	}
	
	maxd = -1;
	
	FD_ZERO(fdset);
	add_fd(fileno(stdin), fdset);
	add_fd(*udp_sock, fdset);

	set_rate();
	init_index();
	retx_init();
	
	return 0;
}

int udp_handler(int udp_sock, const struct sockaddr_in *bs_addr, fd_set *allset, struct sp_checker_info *spchinfo, struct peer_list_ch_info *plchinfo) {
	struct packet tmp_pck;
	struct sockaddr_in addr;
	int len = sizeof(struct sockaddr_in); 
	struct sockaddr_in *addr_list;
	int error;
	int list_len;
	
	
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
			join_peer(udp_sock, &addr, &tmp_pck, plchinfo);
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
			addr_list = get_sp_list(udp_sock, bs_addr, &list_len, &error);
			if (error) {
				fprintf(stderr, "udp_handler - can't get superpeer address list from bootstrap\n");
				return -1;
			}
			
		 	printf("provo init sp\n");
			init_superpeer(allset, addr_list,list_len);
			
		 	printf("invio register\n");
		 	new_register_packet(&tmp_pck, get_index());
			if (retx_send(udp_sock, bs_addr, &tmp_pck) < 0) {
				perror("errore in udp_send");
			}
		}
		
	}

	return 0;
}

int main (int argc,char *argv[]){
	int i; //lunghezza indirizzi ricevuti
	int ready;
	int udp_sock;
	fd_set rset, allset;

	struct sockaddr_in sp_addr;
	struct sockaddr_in bs_addr;
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

	if (init(&udp_sock, &allset) < 0) {
		fprintf(stderr, "Errore durante l'inizializzazione\n");
		return 1;
	}
	
	//provo a entrare nella rete
	if (start_process(udp_sock, &allset, &sp_addr, &bs_addr, peer_rate) < 0) {
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
			udp_handler(udp_sock, &bs_addr, &allset, sp_check_info, pl_check_info);
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
				end_process(&allset, fd, sp_check_info, pl_check_info, &ping_info);
				pl_check_info = NULL;
				sp_check_info = NULL;

				start_process(udp_sock, &allset, &sp_addr, &bs_addr, peer_rate);

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

