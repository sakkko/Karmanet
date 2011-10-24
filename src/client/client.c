#include "client.h"

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
*	Funzione che riconosce ed esegue i comandi dati da tastiera
*/
int keyboard_handler(int udp_sock){
	struct packet pck; 
	struct addr_node *results;
	char str[MAXLINE];
	int nread;

	if ((nread = readline(fileno(stdin), str, MAXLINE)) < 0) {
		perror("keyboard_handler error - readline failed");
		return -1;
	}

	str[nread - 1] = 0;	

	if (!strncmp(str, "leave", 5)) {
		new_leave_packet(&pck, get_index());
		retx_send(udp_sock, &pinger.addr_to_ping, &pck);
		state = ST_LEAVE_SENT;
	} else if (!strncmp(str, "whohas_md5", 10)) {
		if (strlen(str) <= 11) {
			return 0;
		}
		printf("MD5 richiesto: %s\n", str + 11);	
	} else if (!strncmp(str, "whohas", 6)) {
		if (strlen(str) <= 7) {
			return 0;
		}
		new_whs_query_packet(&pck, get_index(), str + 7, strlen(str) - 7, DEFAULT_TTL);
		pck.data[pck.data_len] = 0;
		printf("File richiesto: %s\n", pck.data);	
		if (is_sp) {
			results = get_by_name(str + 7);
			print_results_name(results, str + 7);
			if (whohas_request_handler(-1, udp_sock, &pck, &myaddr, 0) < 0) {
				fprintf(stderr, "keyboard_handler error - whohas_request_handler failed\n");
				return -1;
			}
		} else {
			if (retx_send(udp_sock, &pinger.addr_to_ping, &pck) < 0) {
				fprintf(stderr, "keyboadr_handler error - retx_send failed\n");
				return -1;
			}			
			//printf("Ricerca iniziata, attendere...\n");
		}
		
	} else if (!strcmp(str, "help")) {
		write_help();
	} else if (!strcmp(str, "update")) {
		if (is_sp) {
			create_diff(conf.share_folder);
			add_sp_file(&myaddr);
		} else {
			create_diff(conf.share_folder);
			send_share(udp_sock, &pinger.addr_to_ping);
		}
	} else if (!strcmp(str, "print_files")) {
		if (is_sp) {
			print_file_table();
		} else {
			printf("Non sei superpeer\n");
		}
	} else if (!strcmp(str, "print_ip")) {
		if (is_sp) {
			print_ip_table();
		} else {
			printf("Non sei superpeer\n");
		}
	} else if (!strcmp(str, "print_md5")) {
		if (is_sp) {
			print_md5_table();
		} else {
			printf("Non sei superpeer\n");
		}
	} else {
		printf("Comando non riconosciuto, digitare help per la lista dei comandi\n");
	}
	return 0;
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

	init_index();
	set_rate();
	retx.pipe_mutex = &pipe_mutex;
	retx.retx_wr_pipe = thread_pipe[1];

	if (retx_run(&retx) < 0) {
		fprintf(stderr, "init error - can't initialize retransmission\n");
		return -1;
	}

	bserror = 0;
	state = 0;
	fd_init();	
	fd_add(fileno(stdin));
	fd_add(udp_sock);
	fd_add(thread_pipe[0]);

	unlink(FILE_SHARE);

	sigemptyset(&actions.sa_mask);
	actions.sa_flags = 0;
	actions.sa_handler = sighand;

	if (sigaction(SIGALRM, &actions, NULL) < 0) {
		perror("init error - sigaction failed");
		return 1;
	}
	
	return 0;
}

/*
 * Funzione che gestisce il comando ping.
 * Ritorna 0 in caso di successo e -1 in caso di errore.
 */
int ping_handler(int udp_sock, const struct sockaddr_in *addr, const struct packet *recv_pck) {
	struct packet tmp_pck;

	if (is_sp) {
//		printf("Ricevuto ping da %s:%d\n", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
		update_peer_flag(peer_list_checker, addr);
		new_pong_packet(&tmp_pck, recv_pck->index);
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
		update_sp_flag(sp_checker);
	}
}

/**
 * Invia leave a tutti i peer connessi.
 */
void send_leave(int udp_sock) {
	struct node *tmp_node;
	struct packet tmp_pck;

	tmp_node = peer_list_head;

	while (tmp_node != NULL) {
		new_leave_packet(&tmp_pck, get_index());
		if (mutex_send(udp_sock, &((struct peer_node *)tmp_node->data)->peer_addr, &tmp_pck) < 0) {
			fprintf(stderr, "ack_handler error - mutex_send failed\n");
		}
		tmp_node = tmp_node->next;
	}
}

/*
 * Funzione che gestisce il comando ack.
 * Ritorna 0 in caso di successo e -1 in caso di errore.
 */
int ack_handler(int udp_sock, const struct sockaddr_in *addr, const struct sockaddr_in *bs_addr, const struct packet *recv_pck) {
	char cmdstr[CMD_STR_LEN + 1];

	if (retx_stop(recv_pck->index, cmdstr) < 0) {
		fprintf(stderr, "ack_handler error - retx_stop failed\n");
		return -1;
	}

	if (state == ST_JOINSP_SENT) {
		run_threads(udp_sock, NULL, addr);	
		printf("Sono peer, il mio superpeer è %s:%d\n", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
		state = ST_ACTIVE;
		free(addr_list);
		addr_list = NULL;
		create_diff(conf.share_folder);
		send_share(udp_sock, addr);
	} else if (state == ST_PROMOTE_SENT) {
		addrcpy(&child_addr, addr);
		have_child = 1;
		state = ST_ACTIVE;
	} else if (state == ST_REGISTER_SENT) {
		struct packet pck;
		new_leave_packet(&pck, get_index());
		retx_send(udp_sock, &pinger.addr_to_ping ,&pck);
		stop_threads(0);
		is_sp = 1;
		run_threads(udp_sock, bs_addr, NULL);
		create_diff(conf.share_folder);
		add_sp_file(&myaddr);
		state = ST_ACTIVE;
	} else if (state == ST_FILELIST_SENT) {
		send_share(udp_sock, addr);
	} else if (state == ST_LEAVE_SENT) {
		if (is_sp) {
			send_leave(udp_sock);
		}
		exit(0);
	} else {
	}

	return 0;
}

int update_file_list_handler(int udp_sock, const struct sockaddr_in *addr, const struct packet *recv_pck) {
	struct packet send_pck;

	if (add_files(addr, recv_pck->data, recv_pck->data_len) < 0) {
		fprintf(stderr, "file_list_handler error - add files faield\n");
		return -1;
	} else {
		new_ack_packet(&send_pck, recv_pck->index);
		if (mutex_send(udp_sock, addr, &send_pck) < 0) {
			fprintf(stderr, "file_list_handler error - mutex_send faield\n");
			return -1;
		}
	}

	return 0;
}

/*
 * Aggiunge i file condivisi dal superpeer all'hashtable.
 */
int add_sp_file(const struct sockaddr_in *addr) {
	int fd;
	int n;
	char buf[265];

	if ((fd = open(FILE_DIFF, O_RDONLY)) < 0) {
		perror("add_sp_file error - open failed");
		return -1;
	}

	while (1) {
		if ((n = read(fd, buf, MD5_DIGEST_LENGTH + 1)) < 0) {
			perror("add_sp_file error - read failed");
			return -1;
		}
		if (n == 0) {
			break;
		} else if (n != MD5_DIGEST_LENGTH + 1 || (*buf != '+' && *buf != '-')) {
			fprintf(stderr, "add_sp_file error - bad file format\n");
			return -1;
		} 

		if ((n = readline(fd, buf + MD5_DIGEST_LENGTH + 1, 255)) < 0) {
			fprintf(stderr, "add_sp_file error - readline failed\n");
			return -1;
		}
		buf[MD5_DIGEST_LENGTH + n] = 0;
	//	printf("%s\n", buf + MD5_DIGEST_LENGTH + 1);
		
		if (*buf == '+') {
			insert_file(buf + MD5_DIGEST_LENGTH + 1, (const unsigned char *)(buf + 1), addr->sin_addr.s_addr, addr->sin_port);
		} else {
			remove_file(addr->sin_addr.s_addr, addr->sin_port, buf + MD5_DIGEST_LENGTH + 1, (const unsigned char *)buf + 1);
		}

	}

	if (close(fd) < 0) {
		perror("add_sp_file error - close failed");
		return -1;
	}


	return 0;	
}

/*
 * Invia la lista dei file condivisi al superpeer.
 */
int send_share(int udp_sock, const struct sockaddr_in *addr) {
	int fd;
	struct packet pck;
	char buf[1024];
	char tmp[1024];
	int n, rc, count = 0;

	if ((fd = open(FILE_DIFF, O_RDONLY)) < 0) {
		perror("send_share_file error - open failed");
		return -1;
	}

	while (1) {
		if ((n = read(fd, buf, MD5_DIGEST_LENGTH + 1)) < 0) {
			perror("send_share error - read failed");
			return -1;
		} else if (n == 0) {
			if (count > 0) {
				new_packet(&pck, CMD_UPDATE_FILE_LIST, get_index(), tmp, count, 0);
				if ((rc = retx_send(udp_sock, addr, &pck)) < 0) {
					fprintf(stderr, "send_share error - try_retx_send failed\n");
					if (close(fd) < 0) {
						perror("send_share error - close failed");
					}
					return -1;
				}
				count = 0;
			}
			break;
		} else if (n != MD5_DIGEST_LENGTH + 1) {
			fprintf(stderr, "send_share - bad file format\n");
			return -1;
		} else {
			if (count + n < 1024) {
				if ((n = readline(fd, buf + MD5_DIGEST_LENGTH + 1, 255)) < 0) {
					fprintf(stderr, "send_share error - readline failed\n");
					return -1;
				} else if (n == 0) {
					fprintf(stderr, "send_share error - bad file format\n");
					return -1;
				} else {
					if (count + n + MD5_DIGEST_LENGTH + 1 < 1024) {
						memcpy(tmp + count, buf, count + n + MD5_DIGEST_LENGTH + 1);	
						count += n + MD5_DIGEST_LENGTH + 1;
					} else {
						new_packet(&pck, CMD_UPDATE_FILE_LIST, get_index(), tmp, count, 0);
						if ((rc = retx_send(udp_sock, addr, &pck)) < 0) {
							if (close(fd) < 0) {
								perror("send_share error - close failed");
							}
							fprintf(stderr, "send_share error - try_retx_send failed\n");
							return -1;
						}
						lseek(fd, -1 * (n + MD5_DIGEST_LENGTH + 1)  , SEEK_CUR);
						count = 0;
					}
				}
			} else {
				new_packet(&pck, CMD_UPDATE_FILE_LIST, get_index(), tmp, count, 0);
				if ((rc = retx_send(udp_sock, addr, &pck)) < 0) {
					if (close(fd) < 0) {
						perror("send_share error - close failed");
					}
					fprintf(stderr, "send_share error - try_retx_send failed\n");
					return -1;
				}
				lseek(fd, -1 * n, SEEK_CUR);
				count = 0;
			}
			
		}

	}

	state = ST_ACTIVE;

	if (close(fd) < 0) {
		perror("send_share_file error - close failed");
		return -1;
	}

	return 0;

}

/*
 * Gestisce la ricezione del comando leave.
 */
int leave_handler(int udp_sock, const struct packet *recv_pck, const struct sockaddr_in *bs_addr, const struct sockaddr_in *addr) {
	struct packet tmp_pck;

	if (is_sp) {
		printf("Elimino peer %s\n", inet_ntoa(addr->sin_addr));
		remove_peer(addr);
		remove_all_file(addr->sin_addr.s_addr, addr->sin_port);
		new_ack_packet(&tmp_pck, recv_pck->index);

		if (mutex_send(udp_sock, addr, &tmp_pck) < 0) {
			fprintf(stderr, "leave_handler error - mutex_send failed\n");
			return -1;
		}		
	} else {
		if (addrcmp(&pinger.addr_to_ping, addr)) {
			stop_threads(0);
			unlink(FILE_SHARE);
			new_join_packet(&tmp_pck, get_index());
			if (retx_send(udp_sock, bs_addr, &tmp_pck) < 0) {
				fprintf(stderr, "leave_handler error - Can't send join to bootstrap\n");
				return 1;
			}

			state = ST_JOINBS_SENT;
		}
	}

	return 0;

}

void help(char *progname) {
	usage(progname);
	write_help();
}

void usage(char *progname) {
	printf("Utilizzo: %s <indirizzo IP server bootstrap>\n", progname);
	printf("Utilizzare: %s --help per maggiori informazioni\n", progname);
}

void write_help() {
	printf("Lista comandi:\n");
	printf("\t-whohas: effettua la ricerca per nome\n");
	printf("\t-whohas_md5: effettua la ricerca per md5\n");
	printf("\t-leave: chiude il programma\n");
	printf("\t-get: scarica un file\n");
	printf("\t-update: aggiorna e invia la lista dei file condivisi\n");
	if (is_sp) {
		printf("\t-print_files: stampa la lista dei file\n");
		printf("\t-print_ip: stampa la lista degli ip\n");
		printf("\t-print_md5: stampa la lista degli md5\n");
	}
}


/*
 * Gestisce il messaggio di richiesta whohas, inoltra la richiesta sulla rete overlay
 * e invia i risultati al peer che ha fatto la richiesta.
 */
int whohas_request_handler(int socksd, int udp_sock, const struct packet *pck, const struct sockaddr_in *addr, int overlay) {
	struct addr_node *results;
	struct packet tmp_pck, send_pck;
	char buf[MAX_PACKET_DATA_LEN];
	int offset;
	
	pckcpy(&tmp_pck, pck);
	if (overlay == 0) {
		//il whohas è arrivato da un peer (socket udp)
		addr2str(tmp_pck.data, addr->sin_addr.s_addr, addr->sin_port);
		memcpy(tmp_pck.data + ADDR_STR_LEN, pck->data, pck->data_len);
		tmp_pck.data_len += ADDR_STR_LEN;
	}
	
	tmp_pck.data[tmp_pck.data_len] = 0;


	if (flood_overlay(&tmp_pck, socksd) < 0) {
		fprintf(stderr, "whohas_handler error - flood_overlay failed\n");
		return -1;
	}

	if (addrcmp(&myaddr, addr)) {
		return 0;
	}

	if (is_set_flag(&tmp_pck, PACKET_FLAG_WHOHAS_NAME)) {
		results = get_by_name(tmp_pck.data + ADDR_STR_LEN);
		strcpy(buf, tmp_pck.data + ADDR_STR_LEN);
		buf[tmp_pck.data_len - ADDR_STR_LEN] = '\n';
		offset = tmp_pck.data_len - ADDR_STR_LEN + 1;

		while (results != NULL) {
			if (offset + ADDR_STR_LEN + MD5_DIGEST_LENGTH < MAX_PACKET_DATA_LEN) {
				addr2str(buf + offset, results->ip, results->port);
				offset += ADDR_STR_LEN;
				memcpy(buf + offset, results->md5->md5, MD5_DIGEST_LENGTH);
				offset += MD5_DIGEST_LENGTH;
				if (results->next == NULL) {
					new_whs_res_packet(&send_pck, tmp_pck.index, buf, offset, 1);
					if (retx_send(udp_sock, addr, &send_pck) < 0) {
						fprintf(stderr, "whohas_handler error - retx_send failed\n");
						return -1;
					}
					break;
				} else {
					results = results->next;	
				}
			} else {
				new_whs_res_packet(&send_pck, tmp_pck.index, buf, offset, 1);
				if (retx_send(udp_sock, addr, &send_pck) < 0) {
					fprintf(stderr, "whohas_handler error - retx_send failed\n");
					return -1;
				}
				offset = tmp_pck.data_len - ADDR_STR_LEN + 1;
			}

		}


	} else if (is_set_flag(&tmp_pck, PACKET_FLAG_WHOHAS_MD5)) {

	}
	

	return 0;
}

/*
 * Gestisce il messaggio di risposta whohas.
 */
int whohas_response_handler(int udp_sock, const struct packet *pck, const struct sockaddr_in *addr) {
	char *tmp;
	char md5_hex[MD5_DIGEST_LENGTH * 2 + 1];
	int offset;
	struct packet send_pck;
	unsigned long ip;
	unsigned short port;
	struct in_addr inaddr;

	new_ack_packet(&send_pck, pck->index);
	if (mutex_send(udp_sock, addr, &send_pck) < 0) {
		fprintf(stderr, "udp_handler error - mutex_send failed\n");
		return -1;
	}

	if (is_set_flag(pck, PACKET_FLAG_WHOHAS_NAME)) {
		if ((tmp = strstr(pck->data, "\n")) == NULL) {
			fprintf(stderr, "whohas_handler error - bad packet format\n");
			return -1;
		}
		*tmp = 0;

		offset = tmp - pck->data + 1;

		while (offset < pck->data_len) {
			ip = btol(pck->data + offset);
			inaddr.s_addr = ip;
			offset += sizeof(long);
			port = btos(pck->data + offset);
			offset += sizeof(short);
			to_hex(md5_hex, (unsigned char *)(pck->data + offset));
			md5_hex[MD5_DIGEST_LENGTH * 2] = 0;
			offset += MD5_DIGEST_LENGTH;

			printf("%s:%u %s %s\n", inet_ntoa(inaddr), ntohs(port), md5_hex, pck->data);
		}

	} else if (is_set_flag(pck, PACKET_FLAG_WHOHAS_MD5)) {

	}


	return 0;
}

/*
 * Gestisce il messaggio whohas ricevuto sulla socket udp.
 */
int whohas_handler_udp(int socksd, const struct packet *pck, const struct sockaddr_in *addr) {
	struct packet send_pck;

	if (is_set_flag(pck, PACKET_FLAG_WHOHAS_QUERY)) {
		//QUERY
		if (is_sp) {
			new_ack_packet(&send_pck, pck->index);
			if (mutex_send(socksd, addr, &send_pck) < 0) {
				fprintf(stderr, "udp_handler error - mutex_send failed\n");
				return -1;
			}
			if (whohas_request_handler(-1, socksd, pck, addr, 0) < 0) {
				fprintf(stderr, "whohas_handler_udp error - whohas_request_handler failed\n");
				return -1;
			}
		} else {
			//NO SONO SP, NON POSSO GESTIE LE RICHIESTE WHOHAS
			new_err_packet(&send_pck, pck->index);
			if (mutex_send(socksd, addr, &send_pck) < 0) {
				fprintf(stderr, "whohas_handler_udp error - mutex_send failed\n");
				return -1;
			}
		}
	} else {
		//RISPOSTA
		if (whohas_response_handler(socksd, pck, addr) < 0) {
			fprintf(stderr, "whohas_handler_udp error - whohas_response_handler failed\n");
			return -1;
		}
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
	
	if (recvfrom_packet(udp_sock, &addr, &recv_pck, &len) < 0) {
		printf("ERRORE RECV_PACKET\n");
		return -1;
	}

	//TODO se ho già processato questa richiesta nel immediato passato rinvio l'ack
	
//	printf("PACKET: CMD=%s INDEX=%u\n", recv_pck.cmd, recv_pck.index);
			
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
		if (ping_handler(udp_sock, &addr, &recv_pck) < 0) {
			fprintf(stderr, "udp_handler error - ping_handler failed\n");
		}
	} else if (!strncmp(recv_pck.cmd, CMD_LEAVE, CMD_STR_LEN)) { 
		if (leave_handler(udp_sock, &recv_pck, bs_addr, &addr) < 0) {
			fprintf(stderr, "udp_handler error - leave_handler failed\n");
			return -1;
		}
	} else if (!strncmp(recv_pck.cmd, CMD_WHOHAS, CMD_STR_LEN)) { 
		if (whohas_handler_udp(udp_sock, &recv_pck, &addr) < 0) {
			fprintf(stderr, "udp_handler error - whohas_handler failed\n");
			return -1;
		}
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
	} else if (!strncmp(recv_pck.cmd, CMD_UPDATE_FILE_LIST, CMD_STR_LEN)) {
		if (update_file_list_handler(udp_sock, &addr, &recv_pck) < 0) {
			fprintf(stderr, "udp_handler error - file_list_handler failed\n");
			return -1;
		}
	} else if (!strncmp(recv_pck.cmd, CMD_UPDATE_FILE_LIST, CMD_STR_LEN)) {
		if (update_file_list_handler(udp_sock, &addr, &recv_pck) < 0) {
			fprintf(stderr, "udp_handler error - file_list_handler failed\n");
			return -1;
		}
	} else {
		//comando non riconosciuto
		new_err_packet(&recv_pck, recv_pck.index);
		if (mutex_send(udp_sock, &addr, &recv_pck) < 0) {
			fprintf(stderr, "udp_handler error - mutex_send failed\n");
			return -1;
		}

	}

	return 0;
}

int tcp_handler(int tcp_sock, int udp_sock) {
	int n;
	struct packet pck_rcv;
	struct sockaddr_in addr;
	
	if ((n = recv_packet_tcp(tcp_sock, &pck_rcv)) > 0) {
		if (!strncmp(pck_rcv.cmd , CMD_PING, 3)) {
			if (update_near(tcp_sock, pck_rcv.data, pck_rcv.data_len) < 0) {
				fprintf(stderr, "tcp_handler error - update_near failed\n");
				return -1;
			}
		} else if (!strncmp(pck_rcv.cmd, CMD_WHOHAS, CMD_STR_LEN)) {
			str2addr(&addr, pck_rcv.data);
			if (whohas_request_handler(tcp_sock, udp_sock, &pck_rcv, &addr, 1) < 0) {
				fprintf(stderr, "tcp_handler error - whohas_request_handler failed\n");
				return -1;
			}
		}
	} else if (n == 0) {
		if (close_conn(tcp_sock) < 0) {
			fprintf(stderr, "tcp_handler error - close_conn failed\n");
			return -1;
		}

	} else {
		perror("tcp_handler error - readline failed");
		return -1;
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

	if (retx_stop(recv_pck->index, NULL) < 0) {
		fprintf(stderr, "redirect_handler error - retx_stop failed\n");
		return -1;
	}

	if (state == ST_JOINSP_SENT) {
		str2addr(&addr, recv_pck->data);		
		new_join_packet_rate(&send_pck, get_index(), peer_rate);
		if (retx_send(udp_sock, &addr, &send_pck) < 0) {
			fprintf(stderr, "redirect_handler error - retx_send failed\n");
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
	
	if (retx_stop(recv_pck->index, NULL) < 0) {
		fprintf(stderr, "list_handler error - retx_stop failed\n");
		return -1;
	}

	if (state == ST_JOINBS_SENT) {
		bserror = 0;
		addr_list_len = recv_pck->data_len / 6;
		addr_list = str_to_addr(recv_pck->data, addr_list_len);
		curr_addr_index = 0;
		new_join_packet_rate(&send_pck, get_index(), peer_rate);

		if (retx_send(udp_sock, &addr_list[0], &send_pck) < 0) {
			fprintf(stderr, "udp_handler error - retx_send failed\n");
			return -1;
		}
		state = ST_JOINSP_SENT;
	} else if (state == ST_PROMOTE_RECV) {
		bserror = 0;
		addr_list_len = recv_pck->data_len / 6;
		addr_list = str_to_addr(recv_pck->data, addr_list_len);
		init_superpeer(addr_list, recv_pck->data_len / 6);
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

	
	if (retx_stop(recv_pck->index, NULL) < 0) {
		fprintf(stderr, "promote_handler error - retx_stop failed\n");
		return -1;
	}

	if (state == ST_JOINBS_SENT) {
		printf("\nRICEVUTO PROMOTE DA BS\n");
		state = ST_ACTIVE;
		free(addr_list);
		addr_list = NULL;
		init_superpeer(NULL, 0);
		is_sp = 1;
		run_threads(udp_sock, bs_addr, NULL);
		
		create_diff(conf.share_folder);
		str2addr (&myaddr, recv_pck->data);
		add_sp_file(&myaddr);
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

	//se ha inviato un promote e sta ancora aspettando la risposta, ignora i join che riceve
	if (state == ST_PROMOTE_SENT) {
		return 0;
	}
	if (is_sp == 0) {
		new_err_packet(&tmp_packet, pck->index);
		if (mutex_send(udp_sock, addr, &tmp_packet) < 0) {
			fprintf(stderr, "join_handler error - mutex_send failed\n");
			return -1;
		}
		return 0;
	} else {
		if (get_node_peer(addr) != NULL) {
			new_ack_packet(&tmp_packet, pck->index);
		} else {
			if (curr_p_count < MAX_P_COUNT) {
				//posso accettare il peer
				if (join_peer(addr, btol(pck->data), peer_list_checker) < 0) {
					fprintf(stderr, "join_handler error - can't join peer\n");
					return -1;
				}
				new_ack_packet(&tmp_packet, pck->index);
			} else {
				printf("troppi P!!!\n");
				if (have_child == 0) {
					curr_redirect_count = 0;
					printf("PROMUOVO\n");
					if (promote_peer(udp_sock, peer_list_checker) < 0) {
						fprintf(stderr, "join_handler error - promote_peer failed\n");
						return -1;
					}
					state = ST_PROMOTE_SENT;
					return 0;
				} else {
					printf("DIROTTO\n");
					curr_redirect_count ++;
					if (curr_redirect_count >= MAX_REDIRECT_COUNT) {
						printf("Troppi dirottamenti, al prossimo join promuovo\n");
						have_child = 0;
					}
					new_redirect_packet(&tmp_packet, pck->index, &child_addr);
				}
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
	if (close(thread_pipe[0]) < 0) {
		perror("end_process error - close failed");
		return -1;
	}
	if (close(thread_pipe[1]) < 0) {
		perror("end_process error - close failed");
		return -1;
	}

	fd_remove(thread_pipe[0]);

	return stop_threads(reset);

}

/*
 * Funzione che lancia i thread per il controllo delle liste e l'invio dei ping.
 * Ritorna 0 in caso di successo e -1 in caso di errore.
 */
int run_threads(int udp_sock, const struct sockaddr_in *bs_addr, const struct sockaddr_in *sp_addr) {
	pinger.socksd = udp_sock;

	if (is_sp) {
		//alloco la struttura per il controllo della lista dei peer
		peer_list_checker = (struct peer_list_ch_info *)malloc(sizeof(struct peer_list_ch_info));
		//setto il puntatore alla variabile che contiene l'indirizzo della testa della lista
		peer_list_checker->peer_list = &peer_list_head;
		//faccio partire il thread che controlla la lista
		if (peer_list_checker_run(peer_list_checker) < 0) {
			fprintf(stderr, "Can't star peer list checker\n");
			return -1;
		}
		//imposto l'indirizzo a cui fare il ping
		addrcpy(&pinger.addr_to_ping, bs_addr);
	} else {
		//alloco la struttura per il controllo del superpeer
		sp_checker = (struct sp_checker_info *)malloc(sizeof(struct sp_checker_info));
		//imposto il flag di presenza del superpeer
		sp_checker->sp_flag = 1;
		//imposto la pipe
		sp_checker->sp_checker_pipe = thread_pipe[1];
		sp_checker->pipe_mutex = &pipe_mutex;
		//faccio partire il thread che controlla il flag di presenza del superpeer
		if (sp_checker_run(sp_checker) < 0) {
			fprintf(stderr, "Can't start superpeer checker\n");
			return -1;
		}
		//imposto l'indirizzo a cui fare il ping
		addrcpy(&pinger.addr_to_ping, sp_addr);
	}

	//faccio partire il processo dei ping
	if (pinger_run(&pinger) < 0) {
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
	if (pinger_stop(&pinger) < 0) {
		fprintf(stderr, "stop_threads error - can't stop pinger\n");
		return -1;
	}

	if (is_sp) {
		//interrompo il thread che controlla la lista dei peer
		if (peer_list_checker_stop(peer_list_checker) < 0) {
			fprintf(stderr, "stop_threads error - can't stop peer list checker\n");
			return -1;
		}
		if ((rc = pthread_join(peer_list_checker->thinfo.thread, (void *)&ret_value)) != 0) {
			fprintf(stderr, "stop_threads error - can't join peer list checker thread: %s\n", strerror(rc));
			return -1;
		}
		free(peer_list_checker);
		peer_list_checker = NULL;
	} else {
		if (reset == 0) {
			//interrompo il thread che controlla il superpeer
			if (sp_checker_stop(sp_checker) < 0) {
				fprintf(stderr, "stop_threads error - can't stop superpeer checker\n");
				return -1;
			}
		}
		if ((rc = pthread_join(sp_checker->thinfo.thread, (void *)&ret_value)) != 0) {
			fprintf(stderr, "stop_threads error - can't join superpeer checker thread: %s\n", strerror(rc));
			return -1;
		}
		free(sp_checker);
		sp_checker = NULL;
	}

	if ((rc = pthread_join(pinger.thinfo.thread, (void *)&ret_value)) != 0) {
		fprintf(stderr, "stop_threads error - can't join pinger thread: %s\n", strerror(rc));
		return -1;
	}
	
	
	return 0;
	
}

/*
 * Funzione che gestisce le situazioni di errore segnalate dai thread che effettuano
 * la ritrasmissione.
 * Ritorna 0 in caso di successo e -1 in caso di errore.
 */
int error_handler(int udp_sock, const char *errstr, const struct sockaddr_in *bs_addr) {
	struct packet send_pck;

	if (!strncmp(errstr, CMD_JOIN, CMD_STR_LEN)) {
		if (state == ST_JOINBS_SENT) {
			printf("Impossibile contattare il server di bootstrap\n");
			if (bserror < MAX_BS_ERROR) {
				printf("Riprovo a contattare il server di bootstrap\n");
				bserror ++;
				new_join_packet(&send_pck, get_index());
				if (retx_send(udp_sock, bs_addr, &send_pck) < 0) {
					fprintf(stderr, "error_handler error - retx_send failed\n");
					return -1;
				}
				state = ST_JOINBS_SENT;
			} else {
				printf("Troppi tentativi, server di bootstrap irraggiungibile - Abort\n");
				exit(1);
			}
		} else if (state == ST_JOINSP_SENT) {
			printf("Impossibile contattare superpeer %s:%d\n", inet_ntoa(addr_list[curr_addr_index].sin_addr), ntohs(addr_list[curr_addr_index].sin_port));
			if (curr_addr_index < addr_list_len - 1) {
				printf("Provo a contattare il prossimo della lista\n");
				curr_addr_index ++;
				new_join_packet_rate(&send_pck, get_index(), peer_rate);
				if (retx_send(udp_sock, &addr_list[curr_addr_index], &send_pck) < 0) {
					fprintf(stderr, "error_handler error - retx_send failed\n");
					return -1;
				}
				state = ST_JOINSP_SENT;
			} else {
				printf("Richiedo la lista al bootstrap\n");
				new_join_packet(&send_pck, get_index());
				if (retx_send(udp_sock, bs_addr, &send_pck) < 0) {
					fprintf(stderr, "error_handler error - retx_send failed\n");
					return -1;
				}
				state = ST_JOINBS_SENT;
			}
		}
	} else if (!strncmp(errstr, CMD_PROMOTE, CMD_STR_LEN)) {
		printf("Impossibile inviare promote al best peer\n");
		if (curr_p_count >= MAX_P_COUNT) {
			printf("Riprovo a promuovere il best peer\n");
			if (promote_peer(udp_sock, peer_list_checker) < 0) {
				fprintf(stderr, "error_handler error - promote_peer failed\n");
				return -1;
			}
			state = ST_PROMOTE_SENT;

		} else {
			printf("Promote non più necessario, rinuncio\n");
			state = ST_ACTIVE;
		}
	} else if (!strncmp(errstr, CMD_LEAVE, CMD_STR_LEN)) {
		exit(0);
	}
	return 0;
}

int pipe_handler(int udp_sock, const struct sockaddr_in *addr) {
	int nread;
	char str[MAXLINE];
	struct packet tmp_pck;

	if ((nread = readline(thread_pipe[0], str, MAXLINE)) < 0) {
		fprintf(stderr, "pipe_handler error - read failed\n");
		return 1;
	}
	str[nread - 1] = 0;
	printf("pipe: %s\n", str);
	if (!strncmp(str, "RST", 3)) {
		stop_threads(1);
		new_join_packet(&tmp_pck, get_index());
		if (retx_send(udp_sock, addr, &tmp_pck) < 0) {
			fprintf(stderr, "pipe_handler error - retx_send failed\n");
			return 1;
		}

		state = ST_JOINBS_SENT;
	} else if (!strncmp(str, "ERR", 3)) {
		//si è verificato un errore
		if (error_handler(udp_sock, str + 4, addr) < 0) {
			fprintf(stderr, "pipe_handler error - error_handler failed\n");
			return 1;
		}
	}

	return 0;
}

int main(int argc,char *argv[]) {
	int udp_sock, ready;
	struct near_node *iterator;

	char cwd[MAXLINE];

	struct sockaddr_in addr;
	struct packet tmp_pck;

	if (argc != 2) { 
		usage(argv[0]);
		return 1;
	}

	if (!strcmp("--help", argv[1])) {
		help(argv[0]);
		return 0;
	}

	get_dirpath(cwd, argv[0]);
	if (chdir(cwd) < 0) {
		perror("Can't change current workink directory");
		return 1;
	}

	if ((udp_sock = udp_socket()) < 0) {
		perror("Can't initialize socket");
		return -1;
	}

	if (read_config(&conf) < 0) {
		fprintf(stderr, "Can't read configuration file\n");
		return -1;
	}

	set_addr_any(&addr, conf.udp_port);	

	if (inet_bind(udp_sock, &addr) < 0) {
		perror("inet_bind failed");
		return 1;
	}

	if (conf.udp_port == 0) {
		conf.udp_port = get_local_port(udp_sock);
		printf("PORT = %u\n", ntohs(conf.udp_port));
	}

	if (set_addr_in(&addr, argv[1], BS_PORT) < 0) {
		fprintf(stderr, "Invalid address %s", argv[1]);
		return 1;
	}	

	if (init(udp_sock) < 0) {
		fprintf(stderr, "init failed\n");
		return 1;
	}
	

	new_join_packet(&tmp_pck, get_index());
	if (retx_send(udp_sock, &addr, &tmp_pck) < 0) {
		fprintf(stderr, "Can't send join to bootstrap\n");
		return 1;
	}

	state = ST_JOINBS_SENT;

	//main loop - select	
	while (1) {
		if ((ready = fd_select()) < 0) {
      		perror("fd_select failed");
			return 1;
		}
	    	
		if (fd_ready(udp_sock)) { 
			if (udp_handler(udp_sock, &addr) < 0) {
				fprintf(stderr, "udp_handler failed\n");
			}
		} else if (fd_ready(fileno(stdin))) {
			if (keyboard_handler(udp_sock) < 0) {
				fprintf(stderr, "keyboard_handler failed\n");
				return 1;
			}
		} else if (fd_ready(tcp_listen)) {
			if (accept_conn(tcp_listen) < 0) {
				fprintf(stderr, "accept_conn failed\n");
				return 1;
			}
		} else if (fd_ready(thread_pipe[0])) {
			if (pipe_handler(udp_sock, &addr) < 0) {
				fprintf(stderr, "pipe_handler failed\n");
				return 1;
			}
		} else {
			iterator = near_list_head;
			while (iterator != NULL) {
				if (fd_ready(iterator->socksd)) {
					if (tcp_handler(iterator->socksd, udp_sock) < 0) {
						fprintf(stderr, "Error\n");
						return 1;
					}
					if (--ready <= 0) {
						break;
					}
				}
				iterator = iterator->next;
			}
		}			
	}

	return 0;
}

