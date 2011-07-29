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
 * Gestore del segnale SIGALRM. Usato solamente per svegliare i thread in sleep.
 */
void sighand(int signo) {
	printf("sighand - sblocco thread\n");
	return;
}

/*
* Funzione di inizializzazione generale.
*/
int init(int udp_sock) {
	struct sigaction actions;

	if (pipe(thread_pipe) < 0) {
		perror("init error - can't initialize pipe");
		return -1;
	}

	set_rate();
	init_index();
	retx_init(thread_pipe[1], &pipe_mutex);

	state = 0;
	fd_init();	
	fd_add(fileno(stdin));
	fd_add(udp_sock);
	fd_add(thread_pipe[0]);

	sigemptyset(&actions.sa_mask);
	actions.sa_flags = 0;
	actions.sa_handler = sighand;

	if (sigaction(SIGALRM, &actions, NULL) < 0) {
		perror("sigaction failed");
		return 1;
	}

	
	return 0;
}

/*
 * Funzione che gestisce il comando ping.
 * Ritorna 0 in caso di successo e -1 in caso di errore.
 */
int ping_handler(int udp_sock, const struct sockaddr_in *addr) {
	struct packet tmp_pck;

	if (is_sp) {
		printf("Ricevuto ping da %s:%d\n", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
		update_peer_flag(plchinfo, addr);
		new_pong_packet(&tmp_pck, tmp_pck.index);
		if (mutex_send(udp_sock, addr, &tmp_pck) < 0) {
			fprintf(stderr, "udp_send error - mutex_send failed\n");
			return -1;
		}
	}

	return 0;
}

/*
 * Funzione che gestisce il comando pong.
 */
void pong_handler() {
	if (is_sp == 0) {
		printf("Ricevuto pong da superpeer\n");
		update_sp_flag(spchinfo);
	}
}

/*
 * Funzione che gestisce il comando ack.
 * Ritorna 0 in caso di successo e -1 in caso di errore.
 */
int ack_handler(int udp_sock, const struct sockaddr_in *addr, const struct sockaddr_in *bs_addr, const struct packet *recv_pck) {
	if (state == ST_JOINSP_SENT) {
		retx_stop(recv_pck->index);
		run_threads(udp_sock, NULL, addr);	
		printf("Sono peer, il mio superpeer è %s:%d\n", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
		state = ST_ACTIVE;
		free(addr_list);
		addr_list = NULL;
	} else if (state == ST_PROMOTE_SENT) {
		retx_stop(recv_pck->index);
		addrcpy(&child_addr, addr);
		have_child = 1;
		state = ST_ACTIVE;
	} else if (state == ST_REGISTER_SENT) {
		retx_stop(recv_pck->index);	
		stop_threads(0);
		is_sp = 1;
		run_threads(udp_sock, bs_addr, NULL);
	}

	return 0;
}

/*
 * Funzione che gestisce tutti i messaggi ricevuti sulla socket udp.
 * Ritorna 0 in caso di successo e -1 in caso di errore.
 */
int udp_handler(int udp_sock, const struct sockaddr_in *bs_addr) {
	struct packet recv_pck;
	struct sockaddr_in addr;
	int len = sizeof(struct sockaddr_in); 
	
	printf("========= UDP_HANDLER ==============\n");
	
	if (recvfrom_packet(udp_sock, &addr, &recv_pck, &len) < 0) {
		printf("ERRORE RECV_PACKET\n");
		return -1;
	}

	//TODO se ho già processato questa richiesta nel immediato passato rinvio l'ack
	
	printf("PACKET: CMD=%s INDEX=%u\n", recv_pck.cmd, recv_pck.index);
			
	if (!strncmp(recv_pck.cmd, CMD_ACK, CMD_STR_LEN)) {
		//ricevuto ack
		if (ack_handler(udp_sock, &addr, bs_addr, &recv_pck) < 0) {
			fprintf(stderr, "udp_handler error - ack_handler failed\n");
			return -1;
		}
	} else if (!strncmp(recv_pck.cmd, CMD_JOIN, CMD_STR_LEN)) {
		if (join_handler(udp_sock, &addr, &recv_pck) < 0) {
			fprintf(stderr, "udp_handler error - join error\n");
			return -1;
		}
	} else if (!strncmp(recv_pck.cmd, CMD_ERR, CMD_STR_LEN)) {
		//ricevuto ERR
		;
	} else if (!strncmp(recv_pck.cmd, CMD_PING, CMD_STR_LEN)) { 
		if (ping_handler(udp_sock, &addr) < 0) {
			fprintf(stderr, "udp_handler error - ping_handler failed\n");
		}
	} else if (!strncmp(recv_pck.cmd, CMD_LEAVE, CMD_STR_LEN)) { 
		//rimuovi addr
	} else if (!strncmp(recv_pck.cmd, CMD_WHOHAS, CMD_STR_LEN)) { 
		;
	} else if (!strncmp(recv_pck.cmd, CMD_PONG, CMD_STR_LEN)) { 
		pong_handler();
	} else if (!strncmp(recv_pck.cmd, CMD_PROMOTE, CMD_STR_LEN)) { 
		if (promote_handler(udp_sock, &addr, bs_addr, &recv_pck) < 0) {
			fprintf(stderr, "udp_handler error - promote_handler failed\n");
			return -1;
		}
	} else if (!strncmp(recv_pck.cmd, CMD_LIST, CMD_STR_LEN)) {
		if (list_handler(udp_sock, bs_addr, &recv_pck) < 0) {
			fprintf(stderr, "udp_handler error - list_handler failed\n");
			return -1;
		}
	} else if (!strncmp(recv_pck.cmd, CMD_REDIRECT, CMD_STR_LEN)) {
		if (redirect_handler(udp_sock, &recv_pck) < 0) {
			fprintf(stderr, "udp_handler error - redirect_handler failed\n");
			return -1;
		}
	} else {
		//comando non riconosciuto

	}

	return 0;
}


/*
 * Funzione che gestisce il comando redirect.
 * Ritorna 0 in caso di successo e -1 in caso di errore.
 */
int redirect_handler(int udp_sock, const struct packet *recv_pck) {
	struct packet send_pck;
	struct sockaddr_in addr;

	if (state == ST_JOINSP_SENT) {
		retx_stop(recv_pck->index);
		str2addr(&addr, recv_pck->data);		
		new_join_packet_rate(&send_pck, get_index(), peer_rate);
		if (retx_send(udp_sock, &addr, &send_pck) < 0) {
			fprintf(stderr, "udp_handler error - retx_send failed\n");
			return -1;
		}
	}

	return 0;
}

/*
 * Funzione che gestisce il comando list.
 * Ritorna 0 in caso di successo e -1 in caso di errore.
 */
int list_handler(int udp_sock, const struct sockaddr_in *bs_addr, const struct packet *recv_pck) {
	struct packet send_pck;

	if (state == ST_JOINBS_SENT) {
		retx_stop(recv_pck->index);
		addr_list = str_to_addr(recv_pck->data, recv_pck->data_len / 6);
		curr_addr_index = 0;
		new_join_packet_rate(&send_pck, get_index(), peer_rate);

		if (retx_send(udp_sock, &addr_list[0], &send_pck) < 0) {
			fprintf(stderr, "udp_handler error - retx_send failed\n");
			return -1;
		}
		state = ST_JOINSP_SENT;
	} else if (state == ST_PROMOTE_RECV) {
		retx_stop(recv_pck->index);
		addr_list = str_to_addr(recv_pck->data, recv_pck->data_len / 6);
		init_superpeer(udp_sock, addr_list, recv_pck->data_len / 6);
		free(addr_list);
		addr_list = NULL;
		new_register_packet(&send_pck, get_index());
		printf("INVIO REGISTER\n");
		if (retx_send(udp_sock, bs_addr, &send_pck) < 0) {
			fprintf(stderr, "udp_handler error - retx_send failed\n");
			return -1;
		}
		state = ST_REGISTER_SENT;
	}
	
	return 0;
}

/*
 * Funzione che gestisce il comando promote.
 * Ritorna 0 in caso di successo e -1 in caso di errore.
 */
int promote_handler(int udp_sock, const struct sockaddr_in *recv_addr, const struct sockaddr_in *bs_addr, const struct packet *recv_pck) {
	struct packet send_pck;

	if (state == ST_JOINBS_SENT) {
		retx_stop(recv_pck->index);
		printf("\nRICEVUTO PROMOTE DA BS\n");
		state = ST_ACTIVE;
		free(addr_list);
		addr_list = NULL;
		init_superpeer(udp_sock, NULL, 0);
		is_sp = 1;
		run_threads(udp_sock, bs_addr, NULL);
	} else {
		if (state == ST_PROMOTE_RECV) {
			return 0;
		}
		if (is_sp || state == ST_REGISTER_SENT) {
			new_ack_packet(&send_pck, recv_pck->index);
			if (mutex_send(udp_sock, recv_addr, &send_pck) < 0) {
				perror ("errore in udp_send");
				return -1;
			}
			return 0;	
		}
		printf("\nRICEVUTO PROMOTE DA SP\n");
		new_join_packet(&send_pck, get_index());
		if (retx_send(udp_sock, bs_addr, &send_pck) < 0) {
			fprintf(stderr, "promote_handler error - retx_send failed\n");
			return -1;
		}
		state = ST_PROMOTE_RECV;
	}

	return 0;

}

/*
 * Funzione che gestisce il comando join.
 * Ritorna 0 in caso di successo e -1 in caso di errore.
 */
int join_handler(int udp_sock, const struct sockaddr_in *addr, const struct packet *pck) {
	struct packet tmp_packet;

	if (is_sp == 0 || state == ST_PROMOTE_SENT) {
		new_err_packet(&tmp_packet, pck->index);
		if (mutex_send(udp_sock, addr, &tmp_packet) < 0) {
			fprintf(stderr, "join_handler error - mutex_send failed\n");
			return -1;
		}
		return 0;
	} else {
		if (curr_p_count < MAX_P_COUNT) {
			//posso accettare il peer
			if (join_peer(addr, btol(pck->data), plchinfo) < 0) {
				fprintf(stderr, "join_handler error - can't join peer\n");
				return -1;
			}
			new_ack_packet(&tmp_packet, pck->index);
		} else {
			printf("troppi P!!!\n");
			if (have_child == 0) {
				printf("PROMUOVO\n");
				if (promote_peer(udp_sock, plchinfo) < 0) {
					fprintf(stderr, "join_handler error - can't promote peer\n");
					return -1;
				}
				state = ST_PROMOTE_SENT;
				insert_request(&child_addr, pck->index);
				return 0;
			} else {
				printf("DIROTTO\n");
				new_redirect_packet(&tmp_packet, pck->index, &child_addr);
			}
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
int end_process(int reset) {
	close(thread_pipe[0]);
	close(thread_pipe[1]);
	fd_remove(thread_pipe[0]);

	return stop_threads(reset);

}

/*
 * Funzione che lancia i thread per il controllo delle liste e l'invio dei ping.
 * Ritorna 0 in caso di successo e -1 in caso di errore.
 */
int run_threads(int udp_sock, const struct sockaddr_in *bs_addr, const struct sockaddr_in *sp_addr) {
	pinfo.socksd = udp_sock;

	if (is_sp) {
		//alloco la struttura per il controllo della lista dei peer
		plchinfo = (struct peer_list_ch_info *)malloc(sizeof(struct peer_list_ch_info));
		//setto il puntatore alla variabile che contiene l'indirizzo della testa della lista
		plchinfo->peer_list = &peer_list_head;
		//faccio partire il thread che controlla la lista
		if (peer_list_checker_run(plchinfo) < 0) {
			fprintf(stderr, "Can't star peer list checker\n");
			return -1;
		}
		//imposto l'indirizzo a cui fare il ping
		addrcpy(&pinfo.addr_to_ping, bs_addr);
	} else {
		//alloco la struttura per il controllo del superpeer
		spchinfo = (struct sp_checker_info *)malloc(sizeof(struct sp_checker_info));
		//imposto il flag di presenza del superpeer
		spchinfo->sp_flag = 1;
		//imposto la pipe
		spchinfo->sp_checker_pipe = thread_pipe[1];
		spchinfo->pipe_mutex = &pipe_mutex;
		//faccio partire il thread che controlla il flag di presenza del superpeer
		if (sp_checker_run(spchinfo) < 0) {
			fprintf(stderr, "Can't start superpeer checker\n");
			return -1;
		}
		//imposto l'indirizzo a cui fare il ping
		addrcpy(&pinfo.addr_to_ping, sp_addr);
	}

	//faccio partire il processo dei ping
	if (pinger_run(&pinfo) < 0) {
		fprintf(stderr, "Can't start ping thread\n");
		return -1;
	}

	return 0;
	
}

/*
 * Funzione che interrompe i thread per il controllo delle liste e l'invio dei ping.
 * La variabile reset indica se il processo di terminazione dei thread è partito a causa
 * di un RST ricevuto sulla pipe, in questo caso il thread che controlla la presenza del
 * superpeer è già terminato e non c'è bisogno di inviargli il segnale; o se è partito
 * a causa di un promote ricevuto dal superpeer, in questo caso il thread che controlla
 * la presenza del superpeer è ancora attivo e bisogna fermarlo.
 * Ritorna 0 in caso di successo e -1 in caso di errore.
 */
int stop_threads(int reset) {
	int ret_value, rc;

	//interrompo il thread che fa i ping
	if (pinger_stop(&pinfo) < 0) {
		fprintf(stderr, "stop_threads error - can't stop pinger\n");
		return -1;
	}

	if (is_sp) {
		//interrompo il thread che controlla la lista dei peer
		if (peer_list_checker_stop(plchinfo) < 0) {
			fprintf(stderr, "stop_threads error - can't stop peer list checker\n");
			return -1;
		}
		if ((rc = pthread_join(plchinfo->thinfo.thread, (void *)&ret_value)) != 0) {
			fprintf(stderr, "stop_threads error - can't join peer list checker thread: %s\n", strerror(rc));
			return -1;
		}
		free(plchinfo);
		plchinfo = NULL;
	} else {
		//interrompo il thread che controlla il superpeer
		if (reset == 0) {
			if (sp_checker_stop(spchinfo) < 0) {
				fprintf(stderr, "stop_threads error - can't stop superpeer checker\n");
				return -1;
			}
		}
		if ((rc = pthread_join(spchinfo->thinfo.thread, (void *)&ret_value)) != 0) {
			fprintf(stderr, "stop_threads error - can't join superpeer checker thread: %s\n", strerror(rc));
			return -1;
		}
		free(spchinfo);
		spchinfo = NULL;
	}

	if ((rc = pthread_join(pinfo.thinfo.thread, (void *)&ret_value)) != 0) {
		fprintf(stderr, "stop_threads error - can't join pinger thread: %s\n", strerror(rc));
		return -1;
	}
	
	
	return 0;
	
}


int main (int argc,char *argv[]){
	int udp_sock, ready;
	int i; //lunghezza indirizzi ricevuti

	char str[MAXLINE];
	int nread;

	struct sockaddr_in bs_addr;
	struct packet tmp_pck;

	// controlla numero degli argomenti
	if (argc != 2) { 
		fprintf(stderr, "Utilizzo: %s <indirizzo IP server>\n", argv[0]);
		return 1;
	}
	
	if ((udp_sock = udp_socket()) < 0) {
		perror("init error - can't initialize socket");
		return -1;
	}

	if (set_addr_in(&bs_addr, argv[1], BS_PORT) < 0) {
		fprintf(stderr, "Invalid address %s", argv[1]);
		return 1;
	}	

	if (init(udp_sock) < 0) {
		fprintf(stderr, "Errore durante l'inizializzazione\n");
		return 1;
	}
	

	new_join_packet(&tmp_pck, get_index());
	if (retx_send(udp_sock, &bs_addr, &tmp_pck) < 0) {
		fprintf(stderr, "Can't send join to bootstrap\n");
		return 1;
	}

	state = ST_JOINBS_SENT;

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
		} else if (fd_ready(thread_pipe[0])) {
			if ((nread = readline(thread_pipe[0], str, MAXLINE)) < 0) {
				fprintf(stderr, "Can't read from pipe\n");
				return 1;
			}
			str[nread - 1] = 0;
			printf("pipe: %s\n", str);
			if (!strncmp(str, "RST", 3)) {
				stop_threads(1);

				new_join_packet(&tmp_pck, get_index());
				if (retx_send(udp_sock, &bs_addr, &tmp_pck) < 0) {
					fprintf(stderr, "Can't send join to bootstrap\n");
					return 1;
				}

				state = ST_JOINBS_SENT;
			} else if (!strncmp(str, "ERR", 3)) {
				//si è verificato un errore
				//TODO gestione dell'errore
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

