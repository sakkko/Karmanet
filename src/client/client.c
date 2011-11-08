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

int do_get(const char *str) {
	char ip_str[15];
	int i;
	unsigned char md5[MD5_DIGEST_LENGTH];
	struct sockaddr_in to_download;
	int len = strlen(str);

	for (i = 0; i < len; i ++) {
		if (str[i] == ':') {
			break;
		}
	}

	if (i == len) {
		fprintf(stderr, "invalid command format. Use get ip:port md5\n");
		return 0;
	}

	strncpy(ip_str, str, i);
	ip_str[i] = 0;
	i ++;

	set_addr_in(&to_download, ip_str, atoi(str + i));

	for ( ; i < len; i ++) {
		if (str[i] == ' ') {
			break;
		}
	}

	if (i == len) {
		fprintf(stderr, "invalid command format. Use get ip:port md5\n");
		return 0;
	}

	i ++;

	if (len - i != MD5_DIGEST_LENGTH * 2) {
		fprintf(stderr, "md5 not valid\n");
		return 0;
	}

	get_from_hex(str + i, md5, MD5_DIGEST_LENGTH);

	if (add_download(&to_download, md5) < 0) {
		fprintf(stderr, "do_get error - add_download failed\n");
		return -1;
	}

	return 0;

}

int do_whohas(int udp_sock, const char *str) {
	struct packet pck; 
	struct addr_node *results;

	new_whs_query_packet(&pck, get_index(), str, strlen(str), DEFAULT_TTL);
	pck.data[pck.data_len] = 0;
	printf("File richiesto: %s\n", pck.data);	
	if (is_sp) {
		results = get_by_name(str);
		print_results_name(results, str);
		if (whohas_request_handler(-1, udp_sock, &pck, &myaddr, 0) < 0) {
			fprintf(stderr, "keyboard_handler error - whohas_request_handler failed\n");
			return -1;
		}
	} else {
		if (retx_send(udp_sock, &pinger.addr_to_ping, &pck) < 0) {
			fprintf(stderr, "keyboadr_handler error - retx_send failed\n");
			return -1;
		}			
	}

	return 0;
}

int do_whohas_md5(int udp_sock, const char *str) {
	struct packet pck; 
	struct md5_info *res_md5;
	unsigned char md5[MD5_DIGEST_LENGTH];

	get_from_hex(str, md5, MD5_DIGEST_LENGTH);
	new_whs_query5_packet(&pck, get_index(), (char *)md5, MD5_DIGEST_LENGTH, DEFAULT_TTL);
	pck.data[pck.data_len] = 0;
	printf("MD5 richiesto: %s\n", str);	
	if (is_sp) {
		res_md5 = get_by_md5(md5);
		print_results_md5(res_md5);
		if (whohas_request_handler(-1, udp_sock, &pck, &myaddr, 0) < 0) {
			fprintf(stderr, "keyboard_handler error - whohas_request_handler failed\n");
			return -1;
		}
	} else {
		if (retx_send(udp_sock, &pinger.addr_to_ping, &pck) < 0) {
			fprintf(stderr, "keyboadr_handler error - retx_send failed\n");
			return -1;
		}			
	}

	return 0;
}

/*
*	Funzione che riconosce ed esegue i comandi dati da tastiera
*/
int keyboard_handler(int udp_sock){
	struct packet pck; 
	char str[MAXLINE];
	int nread;

	if ((nread = readline(fileno(stdin), str, MAXLINE)) < 0) {
		perror("keyboard_handler error - readline failed");
		return -1;
	}

	str[nread - 1] = 0;	

	if (!strncmp(str, "leave", 5)) {
		new_leave_packet(&pck, get_index());
		if (retx_send(udp_sock, &pinger.addr_to_ping, &pck) < 0) {
			fprintf(stderr, "keyboard_handler error - retx_send failed\n");
			return -1;
		}
		state = ST_LEAVE_SENT;
	} else if (!strncmp(str, "whohas_md5", 10)) {
		if (strlen(str) <= 11) {
			return 0;
		}
		if (do_whohas_md5(udp_sock, str + 11) < 0) {
			fprintf(stderr, "keyboard_handler error - do_whohas_md5 failed\n");
			return -1;
		}
	} else if (!strncmp(str, "whohas", 6)) {
		if (strlen(str) <= 7) {
			return 0;
		}
		if (do_whohas(udp_sock, str + 7) < 0) {
			fprintf(stderr, "keyboard_handler error - do_whohas failed\n");
			return -1;
		}
	} else if (!strncmp(str, "get", 3)) {
		if (strlen(str) <= 4) {
			return 0;
		}		
		if (do_get(str + 4) < 0) {
			fprintf(stderr, "keyboard_handler error - do_get faield\n");
			return -1;
		}
	} else if (!strcmp(str, "help")) {
		write_help();
	} else if (!strcmp(str, "update")) {
		if (create_diff(conf.share_folder) < 0) {
			fprintf(stderr, "keyboard_handler error - create_diff failed\n");
			return -1;
		}
		if (is_sp) {
			if (add_sp_file(myaddr.sin_addr.s_addr, conf.tcp_port) < 0) {
				fprintf(stderr, "keyboard_handler error - add_sp_file failed\n");
				return -1;
			}
		} else {
			if (send_share(udp_sock, &pinger.addr_to_ping) < 0) {
				fprintf(stderr, "keyboard_handler error - send_share failed\n");
				return -1;
			}
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

int init_transfer() {

	if (listen(dw_listen_sock, BACKLOG) < 0) {
		perror("init error - listen failed");
		return -1;
	}

	fd_add(dw_listen_sock);

	if (transfer_initialized) {
		return 0;
	}

	download_count = 0;
	upload_count = 0;
	printf("PIPE = %d\n", thread_pipe[1]);
	if (downloader_init(conf.max_download, thread_pipe[1], &pipe_mutex) < 0) {
		fprintf(stderr, "init_tranfer error - downloader_init failed\n");
		return -1;
	}

	if (uploader_init(conf.max_upload, thread_pipe[1], &pipe_mutex) < 0) {
		fprintf(stderr, "init_tranfer error - uploader_init failed\n");
		return -1;
	}

	transfer_initialized = 1;

	return 0;
}

/*
* Funzione di inizializzazione generale.
*/
int init(int udp_sock) {
	struct sigaction actions;
	struct sockaddr_in addr;

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

	if ((dw_listen_sock = tcp_socket()) < 0) {
		perror("init_tranfer error - tcp_socket failed");
		return -1;
	}

	set_addr_any(&addr, conf.tcp_port);
	
	if (inet_bind(dw_listen_sock, &addr) < 0) {
		perror("init_tranfer error - inet_bind failed");
		return -1;
	}

	if (conf.tcp_port == 0) {
		conf.tcp_port = get_local_port(dw_listen_sock);
	} else {
		conf.tcp_port = htons(conf.tcp_port);
	}


	bserror = 0;
	transfer_initialized = 0;
	state = 0;
	nsock = 0;

	fd_init();	
	fd_add(fileno(stdin));
	fd_add(udp_sock);
	fd_add(thread_pipe[0]);

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
		if (update_peer_flag(peer_list_checker, addr) < 0) {
			fprintf(stderr, "ping_handler error - update_peer_flag failed\n");
			return -1;
		}
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
		if (update_sp_flag(sp_checker) < 0) {
			fprintf(stderr, "pong_handler error - update_sp_flag failed\n");
		}
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

int register_handler(int udp_sock, const struct sockaddr_in *addr, const struct packet *recv_pck) {
	struct packet send_pck;
	struct node *tmp_node;

	if (is_sp) {
		if (addrcmp(addr, &child_addr)) {
			new_ack_packet(&send_pck, recv_pck->index);
			if (mutex_send(udp_sock, addr, &send_pck) < 0) {
				fprintf(stderr, "register_handler error - mutex_send failed\n");
				return -1;
			}

			printf("Elimino peer %s:%u\n", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
			if ((tmp_node = get_node_peer(addr)) != NULL) {
				remove_all_file(addr->sin_addr.s_addr, ((struct peer_node *)tmp_node->data)->dw_port);
				remove_peer_node(tmp_node);
			}
			state = ST_ACTIVE;
			have_child = 1;
			return 0;
		}
	}

	new_err_packet(&send_pck, recv_pck->index);
	if (mutex_send(udp_sock, addr, &send_pck) < 0) {
		fprintf(stderr, "register_handler error - mutex_send failed\n");
		return -1;
	}
	
	return 0;
}
/*
 * Funzione che gestisce il comando ack.
 * Ritorna 0 in caso di successo e -1 in caso di errore.
 */
int ack_handler(int udp_sock, const struct sockaddr_in *addr, const struct sockaddr_in *bs_addr, const struct packet *recv_pck) {
	char cmdstr[CMD_STR_LEN + 1];
	struct packet pck;

	if (retx_stop(recv_pck->index, cmdstr) < 0) {
		fprintf(stderr, "ack_handler error - retx_stop failed\n");
		return -1;
	}

	if (state == ST_JOINSP_SENT) {
		if (run_threads(udp_sock, NULL, addr) < 0) {
			fprintf(stderr, "ack_handler error - run_threads failed\n");
			return -1;
		}
		printf("Sono peer, il mio superpeer è %s:%d\n", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
		state = ST_ACTIVE;
		free(addr_list);
		addr_list = NULL;
		if (init_transfer() < 0) {
			fprintf(stderr, "ack_handler error - init_transfer failed\n");
			return -1;
		}
		if (unlink(FILE_SHARE) < 0) {
			perror("ack_handler error - unlink failed");
		}
		if (create_diff(conf.share_folder) < 0) {
			fprintf(stderr, "ack_handler error - create_diff failed\n");
			return -1;
		}
		if (send_share(udp_sock, addr) < 0) {
			fprintf(stderr, "ack_handler error - send_share failed\n");
			return -1;
		}
	} else if (state == ST_PROMOTE_SENT) {
		addrcpy(&child_addr, addr);
	} else if (state == ST_REGISTER_SENT) {
		if (addrcmp(addr, bs_addr)) {
			new_register_packet(&pck, get_index());
			if (retx_send(udp_sock, &pinger.addr_to_ping ,&pck) < 0) {
				fprintf(stderr, "ack_handler error - retx_send failed\n");
				return -1;
			}
			if (stop_threads(0) < 0) {
				fprintf(stderr, "ack_handler error - stop_threads failed\n");
				return -1;
			}
			is_sp = 1;
			run_threads(udp_sock, bs_addr, NULL);
			if (unlink(FILE_SHARE) < 0) {
				perror("ack_handler error - unlink failed");
			}
			if (create_diff(conf.share_folder) < 0) {
				fprintf(stderr, "ack_handler error - create_diff failed\n");
				return -1;
			}
			if (add_sp_file(myaddr.sin_addr.s_addr, conf.tcp_port) < 0) {
				fprintf(stderr, "ack_handler error - add_sp_file failed\n");
				return -1;
			}
			state = ST_ACTIVE;
		}
	} else if (state == ST_LEAVE_SENT) {
		if (is_sp) {
			send_leave(udp_sock);
		}
		exit(0);
	} 

	return 0;
}

int update_file_list_handler(int udp_sock, const struct sockaddr_in *addr, const struct packet *recv_pck) {
	struct packet send_pck;
	struct sockaddr_in new_addr;
	unsigned short port;

	port = *((short *)recv_pck->data);
	addrcpy(&new_addr, addr);
	new_addr.sin_port = port;

	if (add_files(&new_addr, recv_pck->data + sizeof(short), recv_pck->data_len - sizeof(short)) < 0) {
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
int add_sp_file(unsigned long ip, unsigned short port) {
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
		
		if (*buf == '+') {
			insert_file(buf + MD5_DIGEST_LENGTH + 1, (unsigned char *)(buf + 1), ip, port);
		} else {
			remove_file(ip, port, buf + MD5_DIGEST_LENGTH + 1, (unsigned char *)(buf + 1));
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
	char buf[MAX_PACKET_SIZE];
	char tmp[MAX_PACKET_SIZE];
	int n, rc, count = 0;

	if ((fd = open(FILE_DIFF, O_RDONLY)) < 0) {
		perror("send_share_file error - open failed");
		return -1;
	}

	memcpy(tmp, (char *)&conf.tcp_port, sizeof(short));
	while (1) {
		if ((n = read(fd, buf, MD5_DIGEST_LENGTH + 1)) < 0) {
			perror("send_share error - read failed");
			return -1;
		} else if (n == 0) {
			if (count > 0) {
				new_packet(&pck, CMD_UPDATE_FILE_LIST, get_index(), tmp, count + sizeof(short), 0);
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
			if (count + n < MAXLINE - sizeof(short)) {
				if ((n = readline(fd, buf + MD5_DIGEST_LENGTH + 1, 255)) < 0) {
					fprintf(stderr, "send_share error - readline failed\n");
					return -1;
				} else if (n == 0) {
					fprintf(stderr, "send_share error - bad file format\n");
					return -1;
				} else {
					if (count + n + MD5_DIGEST_LENGTH + 1 < MAXLINE - sizeof(short)) {
						memcpy(tmp + count + sizeof(short), buf, n + MD5_DIGEST_LENGTH + 1);	
						count += n + MD5_DIGEST_LENGTH + 1;
					} else {
						new_packet(&pck, CMD_UPDATE_FILE_LIST, get_index(), tmp, count + sizeof(short), 0);
						if ((rc = retx_send(udp_sock, addr, &pck)) < 0) {
							if (close(fd) < 0) {
								perror("send_share error - close failed");
							}
							fprintf(stderr, "send_share error - try_retx_send failed\n");
							return -1;
						}
						lseek(fd, -1 * (n + MD5_DIGEST_LENGTH + 1), SEEK_CUR);
						count = 0;
					}
				}
			} else {
				new_packet(&pck, CMD_UPDATE_FILE_LIST, get_index(), tmp, count + sizeof(short), 0);
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
	struct node *tmp_node;

	if (is_sp) {
		printf("Elimino peer %s:%u\n", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
		if ((tmp_node = get_node_peer(addr)) != NULL) {
			remove_all_file(addr->sin_addr.s_addr, ((struct peer_node *)tmp_node->data)->dw_port);
			remove_peer_node(tmp_node);
		}
		new_ack_packet(&tmp_pck, recv_pck->index);
		if (mutex_send(udp_sock, addr, &tmp_pck) < 0) {
			fprintf(stderr, "leave_handler error - mutex_send failed\n");
			return -1;
		}		
	} else {
		if (addrcmp(&pinger.addr_to_ping, addr)) {
			if (stop_threads(0) < 0) {
				fprintf(stderr, "leave_handler error - stop_threads failed\n");
				return -1;
			}
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

int whohas_request_handler_name(int udp_sock, const struct packet *pck, const struct sockaddr_in *addr) {
	struct addr_node *results;
	struct packet send_pck;
	char buf[MAX_PACKET_DATA_LEN];
	int offset;

	results = get_by_name(pck->data + ADDR_STR_LEN);
	strcpy(buf, pck->data + ADDR_STR_LEN);
	buf[pck->data_len - ADDR_STR_LEN] = '\n';
	offset = pck->data_len - ADDR_STR_LEN + 1;

	while (results != NULL) {
		if (offset + ADDR_STR_LEN + MD5_DIGEST_LENGTH < MAX_PACKET_DATA_LEN) {
			addr2str(buf + offset, results->ip, results->port);
			offset += ADDR_STR_LEN;
			memcpy(buf + offset, results->md5->md5, MD5_DIGEST_LENGTH);
			offset += MD5_DIGEST_LENGTH;
			if (results->next == NULL) {
				new_whs_res_packet(&send_pck, pck->index, buf, offset, 1);
				if (retx_send(udp_sock, addr, &send_pck) < 0) {
					fprintf(stderr, "whohas_request_handler_name error - retx_send failed\n");
					return -1;
				}
				break;
			} else {
				results = results->next;	
			}
		} else {
			new_whs_res_packet(&send_pck, pck->index, buf, offset, 1);
			if (retx_send(udp_sock, addr, &send_pck) < 0) {
				fprintf(stderr, "whohas_request_handler_name error - retx_send failed\n");
				return -1;
			}
			offset = pck->data_len - ADDR_STR_LEN + 1;
		}

	}

	return 0;
}

int whohas_request_handler_md5(int udp_sock, const struct packet *pck, const struct sockaddr_in *addr) {
	struct md5_info *results;
	struct packet send_pck;
	char buf[MAX_PACKET_DATA_LEN];
	int offset;

	print_as_hex((unsigned char *)(pck->data + ADDR_STR_LEN), MD5_DIGEST_LENGTH);
	results = get_by_md5((unsigned char *)(pck->data + ADDR_STR_LEN));
	memcpy(buf, pck->data + ADDR_STR_LEN, MD5_DIGEST_LENGTH);
	offset = MD5_DIGEST_LENGTH;

	while (results != NULL) {
		if (offset + ADDR_STR_LEN + strlen(results->info->file->name) < MAX_PACKET_DATA_LEN) {
			addr2str(buf + offset, results->info->addr->ip, results->info->addr->port);
			offset += ADDR_STR_LEN;
			memcpy(buf + offset, results->info->file->name, strlen(results->info->file->name));
			offset += strlen(results->info->file->name);
			buf[offset] = '\n';
			offset++;
			if (results->next == NULL) {
				new_whs_res5_packet(&send_pck, pck->index, buf, offset, 1);
				if (retx_send(udp_sock, addr, &send_pck) < 0) {
					fprintf(stderr, "whohas_request_handler_md5 error - retx_send failed\n");
					return -1;
				}
				break;
			} else {
				results = results->next;	
			}
		} else {
			new_whs_res5_packet(&send_pck, pck->index, buf, offset, 1);
			if (retx_send(udp_sock, addr, &send_pck) < 0) {
				fprintf(stderr, "whohas_request_handler_md5 error - retx_send failed\n");
				return -1;
			}
			offset = pck->data_len - ADDR_STR_LEN + 1;
		}

	}

	return 0;
}

/*
 * Gestisce il messaggio di richiesta whohas, inoltra la richiesta sulla rete overlay
 * e invia i risultati al peer che ha fatto la richiesta.
 */
int whohas_request_handler(int socksd, int udp_sock, const struct packet *pck, const struct sockaddr_in *addr, int overlay) {
	struct packet tmp_pck;
	int rc, tmp;

	if ((rc = pthread_mutex_lock(&peer_list_checker->request_mutex)) != 0) {
		fprintf(stderr, "whohas_request_handler error - can't acquire lock: %s\n", strerror(rc));
		return -1;
	}

	tmp = insert_request(pck->index, addr->sin_addr.s_addr, addr->sin_port);

	if ((rc = pthread_mutex_unlock(&peer_list_checker->request_mutex)) != 0) {
		fprintf(stderr, "whohas_request_handler error - can't release lock: %s\n", strerror(rc));
		return -1;
	}
	
	if (tmp == 1) {
		printf("RICHIESTA GIA PROCESSATA\n");
		return 0;
	}
	
	pckcpy(&tmp_pck, pck);
	if (overlay == 0) {
		//il whohas è arrivato da un peer (socket udp)
		//inserisco nel pacchetto l'indirizzo del peer che ha generato la richiesta
		addr2str(tmp_pck.data, addr->sin_addr.s_addr, addr->sin_port);
		memcpy(tmp_pck.data + ADDR_STR_LEN, pck->data, pck->data_len);
		tmp_pck.data_len += ADDR_STR_LEN;
	} else {
		tmp_pck.TTL --;
	}
	
	tmp_pck.data[tmp_pck.data_len] = 0;


	if (tmp_pck.TTL > 0) {
		if (flood_overlay(&tmp_pck, socksd) < 0) {
			fprintf(stderr, "whohas_handler error - flood_overlay failed\n");
			return -1;
		}
	}

	if (addrcmp(&myaddr, addr)) {
		return 0;
	}

	if (is_set_flag(&tmp_pck, PACKET_FLAG_WHOHAS_NAME)) {
		if (whohas_request_handler_name(udp_sock, &tmp_pck, addr) < 0) {
			fprintf(stderr, "whohas_request_handler error - whohas_request_handler_name failed\n");
			return -1;
		}
	} else if (is_set_flag(&tmp_pck, PACKET_FLAG_WHOHAS_MD5)) {
		if (whohas_request_handler_md5(udp_sock, &tmp_pck, addr) < 0) {
			fprintf(stderr, "whohas_request_handler error - whohas_request_handler_md5 failed\n");
			return -1;
		}
	}
	

	return 0;
}

int whohas_response_handler_name(const struct packet *pck) {
	char *tmp;
	int offset;
	struct in_addr addr;
	unsigned short port;
	char md5_hex[MD5_DIGEST_LENGTH * 2 + 1];

	if ((tmp = strstr(pck->data, "\n")) == NULL) {
		fprintf(stderr, "whohas_response_handler_name error - bad packet format\n");
		return -1;
	}
	*tmp = 0;

	offset = tmp - pck->data + 1;

	while (offset < pck->data_len) {
		addr.s_addr = btol(pck->data + offset);
		offset += sizeof(long);
		port = btos(pck->data + offset);
		offset += sizeof(short);
		to_hex(md5_hex, (unsigned char *)(pck->data + offset));
		md5_hex[MD5_DIGEST_LENGTH * 2] = 0;
		offset += MD5_DIGEST_LENGTH;

		printf("%s:%u %s %s\n", inet_ntoa(addr), ntohs(port), md5_hex, pck->data);
	}

	return 0;
}

int whohas_response_handler_md5(const struct packet *pck) {
	char md5_hex[MD5_DIGEST_LENGTH * 2 + 1];
	char name[256];
	struct in_addr addr;
	unsigned short port;
	int offset, len;
	char *tmp;

	
	to_hex(md5_hex, (unsigned char *)(pck->data));
	md5_hex[MD5_DIGEST_LENGTH * 2] = 0;	
	offset = MD5_DIGEST_LENGTH;

	while (offset < pck->data_len) {
		addr.s_addr = btol(pck->data + offset);
		offset += sizeof(long);
		port = btos(pck->data + offset);
		offset += sizeof(short);

		if ((tmp = strstr(pck->data + offset, "\n")) == NULL) {
			fprintf(stderr, "whohas_handler error - bad packet format\n");
			return -1;
		}

		len = (int)(tmp - (pck->data + offset));
		strncpy(name, pck->data + offset, len);
		name[len] = 0;
		offset += len + 1;
		printf("%s:%u %s %s\n", inet_ntoa(addr), ntohs(port), md5_hex, name);
	}

	return 0;
}

/*
 * Gestisce il messaggio di risposta whohas.
 */
int whohas_response_handler(int udp_sock, const struct packet *pck, const struct sockaddr_in *addr) {
	struct packet send_pck;

	new_ack_packet(&send_pck, pck->index);
	if (mutex_send(udp_sock, addr, &send_pck) < 0) {
		fprintf(stderr, "udp_handler error - mutex_send failed\n");
		return -1;
	}

	if (is_set_flag(pck, PACKET_FLAG_WHOHAS_NAME)) {
		if (whohas_response_handler_name(pck) < 0) {
			fprintf(stderr, "whohas_response_handler error - whohas_response_handler_name failed\n");
			return -1;
		}
	} else if (is_set_flag(pck, PACKET_FLAG_WHOHAS_MD5)) {
		if (whohas_response_handler_md5(pck) < 0) {
			fprintf(stderr, "whohas_response_handler error - whohas_response_handler_md5 failed\n");
			return -1;
		}
	}

	return 0;
}

/*
 * Gestisce il messaggio whohas ricevuto sulla socket udp.
 */
int whohas_handler_udp(int socksd, const struct packet *pck, const struct sockaddr_in *addr) {
	struct packet send_pck;

	if (is_set_flag(pck, PACKET_FLAG_QUERY)) {
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
			//NO SONO SP, NON POSSO GESTIRE LE RICHIESTE WHOHAS
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

int abort_handler(int udp_sock, const struct sockaddr_in *addr, const struct packet *recv_pck) {
	struct packet send_pck;

	if (is_sp) {
		if (addrcmp(&child_addr, addr)) {
			printf("IMPOSTO STATO ATTIVO\n");
			state = ST_ACTIVE;
		}
		new_ack_packet(&send_pck, recv_pck->index);
		if (mutex_send(udp_sock, addr, &send_pck) < 0) {
			fprintf(stderr, "abort_handler error - mutex_send failed\n");
			return -1;
		}
		return 0;
	}
	new_err_packet(&send_pck, recv_pck->index);
	if (mutex_send(udp_sock, addr, &send_pck) < 0) {
		fprintf(stderr, "abort_handler error - mutex_send failed\n");
		return -1;
	}
	return 0;
}

int err_handler(int udp_sock, const struct sockaddr_in *addr, const struct sockaddr_in *bs_addr, const struct packet *recv_pck) {
	int rc;
	char cmdstr[CMD_STR_LEN + 1];
	struct packet send_pck;

	if ((rc = retx_stop(recv_pck->index, cmdstr)) < 0) {
		fprintf(stderr, "ack_handler error - retx_stop failed\n");
		return -1;
	} else if (rc == 1) {
		return 0;
	}

	if (state == ST_JOINSP_SENT && !strncmp(cmdstr, CMD_JOIN, CMD_STR_LEN)) {
		if (addrcmp(addr, &addr_list[curr_addr_index])) {
			printf("RICEVUTO ERR\n");
			curr_addr_index ++;
			if (curr_addr_index >= addr_list_len) {
				printf("Richiedo la lista al bootstrap\n");
				new_join_packet(&send_pck, get_index());
				if (retx_send(udp_sock, bs_addr, &send_pck) < 0) {
					fprintf(stderr, "err_handler error - retx_send failed\n");
					return -1;
				}
				state = ST_JOINBS_SENT;
			} else {
				new_join_packet_sp(&send_pck, get_index(), peer_rate, conf.tcp_port);
				if (retx_send(udp_sock, &addr_list[curr_addr_index], &send_pck) < 0) {
					fprintf(stderr, "err_handler error - retx_send failed\n");
					return -1;
				}
			}
		}

		return 0;
	} else if (!strncmp(cmdstr, CMD_WHOHAS, CMD_STR_LEN)) {
		if (addrcmp(addr, &pinger.addr_to_ping)) {
			if (state == ST_ACTIVE) {	
				if (stop_threads(0) < 0) {
					fprintf(stderr, "err_handler error - stop_threads failed\n");
					return -1;
				}
				new_join_packet(&send_pck, get_index());
				if (retx_send(udp_sock, bs_addr, &send_pck) < 0) {
					fprintf(stderr, "err_handler error - retx_send failed\n");
					return -1;
				}

				state = ST_JOINBS_SENT;
			}
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

	if (!strncmp(recv_pck.cmd, CMD_ACK, CMD_STR_LEN)) {
		//ricevuto ack
		if (ack_handler(udp_sock, &addr, bs_addr, &recv_pck) < 0) {
			fprintf(stderr, "udp_handler error - ack_handler failed\n");
			return -1;
		}
	} else if (!strncmp(recv_pck.cmd, CMD_JOIN, CMD_STR_LEN)) {
		printf("Ricevuto JOIN\n");
		if (join_handler(udp_sock, &addr, &recv_pck) < 0) {
			fprintf(stderr, "udp_handler error - join error\n");
			return -1;
		}
	} else if (!strncmp(recv_pck.cmd, CMD_ERR, CMD_STR_LEN)) {
		if (err_handler(udp_sock, &addr, bs_addr, &recv_pck) < 0) {
			fprintf(stderr, "udp_handler error - err_handler failed\n");
			return -1;
		}
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
	} else if (!strncmp(recv_pck.cmd, CMD_ABORT, CMD_STR_LEN)) {
		if (abort_handler(udp_sock, &addr, &recv_pck) < 0) {
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
	} else if (!strncmp(recv_pck.cmd, CMD_REGISTER, CMD_STR_LEN)) {
		printf("REGISTER\n");
		if (register_handler(udp_sock, &addr, &recv_pck) < 0) {
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
		if (!strncmp(pck_rcv.cmd , CMD_PING, CMD_STR_LEN)) {
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
		if (errno == ECONNRESET) {
			if (close_conn(tcp_sock) < 0) {
				fprintf(stderr, "tcp_handler error - close_conn failed\n");
				return -1;
			}
			return 0;
		}
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
		new_join_packet_sp(&send_pck, get_index(), peer_rate, conf.tcp_port);
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
	int ret;
	
	if (retx_stop(recv_pck->index, NULL) < 0) {
		fprintf(stderr, "list_handler error - retx_stop failed\n");
		return -1;
	}

	if (state == ST_JOINBS_SENT) {
		bserror = 0;
		free(addr_list);
		addr_list_len = recv_pck->data_len / 6;
		addr_list = str_to_addr(recv_pck->data, addr_list_len);
		curr_addr_index = 0;
		new_join_packet_sp(&send_pck, get_index(), peer_rate, conf.tcp_port);

		if (retx_send(udp_sock, &addr_list[0], &send_pck) < 0) {
			fprintf(stderr, "udp_handler error - retx_send failed\n");
			return -1;
		}
		state = ST_JOINSP_SENT;
	} else if (state == ST_PROMOTE_RECV) {
		bserror = 0;
		free(addr_list);
		addr_list_len = recv_pck->data_len / 6;
		addr_list = str_to_addr(recv_pck->data, addr_list_len);
		printf("ADDRTOSEND = %d\n", recv_pck->data_len / 6);
		ret = init_superpeer(addr_list, recv_pck->data_len / 6);
		free(addr_list);
		addr_list = NULL;
		if (ret < 0) {
			fprintf(stderr, "list_handler error - init_superpeer failed\n");
			state = ST_ACTIVE;
			printf("CHIUDO LA LISTEN SOCK\n");
			if (close(tcp_listen) < 0) {
				perror("bad close");
			}
			new_packet(&send_pck, CMD_ABORT, get_index(), NULL, 0, 1);
			if (retx_send(udp_sock, &pinger.addr_to_ping, &send_pck) < 0) {
				fprintf(stderr, "list_handler error - retx_send failed\n");
				return -1;
			}
			return -1;
		} else if (ret == 1) {
			join_ov_try ++;
			new_join_packet(&send_pck, get_index());
			if (retx_send(udp_sock, bs_addr, &send_pck) < 0) {
				fprintf(stderr, "list_handler error - retx_send failed\n");
				return -1;
			}
			state = ST_OPEN_MORE_CONN;
			return 0;
		}

		new_register_packet(&send_pck, get_index());
		*send_pck.data = (char)(max_tcp_sock - nsock);
		send_pck.data_len = 1;
		printf("INVIO REGISTER\n");
		if (retx_send(udp_sock, bs_addr, &send_pck) < 0) {
			fprintf(stderr, "udp_handler error - retx_send failed\n");
			return -1;
		}
		state = ST_REGISTER_SENT;
	} else if (state == ST_OPEN_MORE_CONN) {
		free(addr_list);
		addr_list_len = recv_pck->data_len / 6;
		addr_list = str_to_addr(recv_pck->data, addr_list_len);
		printf("ADDRTOSEND = %d\n", recv_pck->data_len / 6);
		ret = join_overlay(addr_list, recv_pck->data_len / 6);
		free(addr_list);
		addr_list = NULL;
		if (ret < 0) {
			fprintf(stderr, "list_handler error - join_overlay failed\n");
			printf("CHIUDO LA LISTEN SOCK\n");
			if (close(tcp_listen) < 0) {
				perror("bad close");
			}
			pthread_mutex_destroy(&NEAR_LIST_LOCK);
			fd_remove(tcp_listen);
			free(near_str);
			near_str = NULL;
			new_packet(&send_pck, CMD_ABORT, get_index(), NULL, 0, 1);
			if (retx_send(udp_sock, &pinger.addr_to_ping, &send_pck) < 0) {
				fprintf(stderr, "list_handler error - retx_send failed\n");
				return -1;
			}
			return -1;
		} else if(ret == 1) {
			if (join_ov_try >= MAX_JOIN_OV_TRY) {
				if (nsock == 0) {
					printf("CHIUDO LA LISTEN SOCK\n");
					if (close(tcp_listen) < 0) {
						perror("bad close");
					}
					pthread_mutex_destroy(&NEAR_LIST_LOCK);
					fd_remove(tcp_listen);
					free(near_str);
					near_str = NULL;
					new_packet(&send_pck, CMD_ABORT, get_index(), NULL, 0, 1);
					if (retx_send(udp_sock, &pinger.addr_to_ping, &send_pck) < 0) {
						fprintf(stderr, "list_handler error - retx_send failed\n");
						return -1;
					}
					join_ov_try = 0;
					state = ST_ACTIVE;
					return 0;
				} else {
					new_register_packet(&send_pck, get_index());
					*send_pck.data = (char)(max_tcp_sock - nsock);
					send_pck.data_len = 1;
					if (retx_send(udp_sock, bs_addr, &send_pck) < 0) {
						fprintf(stderr, "list_handler error - retx_send failed\n");
						return -1;
					}
					join_ov_try = 0;
					state = ST_REGISTER_SENT;
					return 0;
				}
			}
			join_ov_try ++;
			new_join_packet(&send_pck, get_index());
			if (retx_send(udp_sock, bs_addr, &send_pck) < 0) {
				fprintf(stderr, "list_handler error - retx_send failed\n");
				return -1;
			}
			state = ST_OPEN_MORE_CONN;
			return 0;
		}

		new_register_packet(&send_pck, get_index());
		*send_pck.data = (char)(max_tcp_sock - nsock);
		send_pck.data_len = 1;
		printf("INVIO REGISTER\n");
		if (retx_send(udp_sock, bs_addr, &send_pck) < 0) {
			fprintf(stderr, "udp_handler error - retx_send failed\n");
			return -1;
		}
		state = ST_REGISTER_SENT;
		join_ov_try = 0;
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
		if (init_superpeer(NULL, 0) < 0) {
			fprintf(stderr, "promote_handler errror - init_superpeer faield\n");
			return -1;
		}
		is_sp = 1;
		if (run_threads(udp_sock, bs_addr, NULL) < 0) {
			fprintf(stderr, "promote_handler errror - run_threads faield\n");
			return -1;
		}

		if (init_transfer() < 0) {
			fprintf(stderr, "promote_handler error - init_transfer failed\n");
			return -1;
		}
		
		if (unlink(FILE_SHARE) < 0) {
			perror("promote_handler error - unlink failed");
		}

		printf("Creo file share, attendere\n");
		if (create_diff(conf.share_folder) < 0) {
			fprintf(stderr, "promote_handler error - create_diff failed\n");
			return -1;
		}
		printf("File share creato\n");
		str2addr(&myaddr, recv_pck->data);
		if (add_sp_file(myaddr.sin_addr.s_addr, conf.tcp_port) < 0) {
			fprintf(stderr, "promote_handler error - add_sp_file failed\n");
			return -1;
		}
	} else {
		new_ack_packet(&send_pck, recv_pck->index);
		if (mutex_send(udp_sock, recv_addr, &send_pck) < 0) {
			perror ("errore in udp_send");
			return -1;
		}
		if (state == ST_PROMOTE_RECV || is_sp || state == ST_REGISTER_SENT) {
			return 0;
		}
		printf("\nRICEVUTO PROMOTE DA SP\n");
		new_join_packet(&send_pck, get_index());
		set_superpeer_flag(&send_pck);
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
				if (join_peer(addr, btol(pck->data), btos(pck->data + sizeof(long)), peer_list_checker) < 0) {
					fprintf(stderr, "join_handler error - can't join peer\n");
					return -1;
				}
				new_ack_packet(&tmp_packet, pck->index);
			} else {
				//se ha inviato un promote e sta ancora aspettando la risposta, ignora il join che riceve
				if (state == ST_PROMOTE_SENT) {
					printf("INVIATO PRM, IGNORO\n");
					return 0;
				}
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
			fprintf(stderr, "Can't start peer list checker\n");
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
	int index;

	printf("ERROR HANDLER\n");

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
				new_join_packet_sp(&send_pck, get_index(), peer_rate, conf.tcp_port);
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
	} else if (!strncmp(errstr, CMD_GET, CMD_STR_LEN)) {
		if (errstr[CMD_STR_LEN] == 'U') {
			index = atoi(errstr + CMD_STR_LEN + 2);
			printf("Errore upload file %s\n", ul_pool[index].ulnode.file_info.md5);
			if (upload_term_handler(index) < 0) {
				fprintf(stderr, "pipe_handler error - upload_term_handler failed\n");
				return -1;
			}
		}
	}

	return 0;
}

int upload_term_handler(int index) {
	int rc;

	if (index < conf.max_upload) {
		if ((rc = pthread_mutex_lock(&ul_pool[index].th_mutex)) != 0) {
			fprintf(stderr, "upload_term_handler error - can't acquire lock: %s\n", strerror(rc));
			return -1;
		}
		ul_pool[index].sleeping = 1;
		if ((rc = pthread_mutex_unlock(&ul_pool[index].th_mutex)) != 0) {
			fprintf(stderr, "upload_term_handler error - can't release lock: %s\n", strerror(rc));
			return -1;
		}
		upload_count --;
		printf("UPLOADS: %d\n", upload_count);
	} else {
		fprintf(stderr, "pipe_handler error - invalid uploader thread index\n");
		return -1;
	}

	return 0;
}

int pipe_handler(int udp_sock, const struct sockaddr_in *addr) {
	int nread;
	char str[MAXLINE];
	struct packet tmp_pck;

	if ((nread = readline(thread_pipe[0], str, MAXLINE)) < 0) {
		fprintf(stderr, "pipe_handler error - read failed\n");
		return -1;
	}
	str[nread - 1] = 0;
	printf("pipe: %s\n", str);
	if (!strncmp(str, "RST", 3)) {
		if (stop_threads(1) < 0) {
			fprintf(stderr, "pipe_handler error - stop_threads failed\n");
			return -1;
		}
		new_join_packet(&tmp_pck, get_index());
		if (retx_send(udp_sock, addr, &tmp_pck) < 0) {
			fprintf(stderr, "pipe_handler error - retx_send failed\n");
			return -1;
		}

		state = ST_JOINBS_SENT;
	} else if (!strncmp(str, "ERR", 3)) {
		//si è verificato un errore
		if (error_handler(udp_sock, str + 4, addr) < 0) {
			fprintf(stderr, "pipe_handler error - error_handler failed\n");
			return -1;
		}
	} else if (!strncmp(str, "TRM", 3)) {
		if (upload_term_handler(atoi(str + 4)) < 0) {
			fprintf(stderr, "pipe_handler error - upload_term_handler failed\n");
			return -1;
		}
	}

	return 0;
}

int main(int argc,char *argv[]) {
	int udp_sock, ready, sock;
	struct near_node *iterator;
	int i;
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

	//imposto la cwd alla cartella contenente i file eseguibili
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
		//la porta è stata scelta dal kernel
		conf.udp_port = get_local_port(udp_sock);
		printf("PORT = %u\n", ntohs(conf.udp_port));
	} else {
		conf.udp_port = htons(conf.udp_port);
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
			//	return 1;
			}
		} else if (fd_ready(tcp_listen)) {
			if (accept_conn(tcp_listen) < 0) {
				fprintf(stderr, "accept_conn failed\n");
			//	return 1;
			}

		} else if (fd_ready(dw_listen_sock)) {
			if ((sock = accept(dw_listen_sock, NULL, NULL)) < 0) {
				perror("accept failed");
				continue;
			}
			if (upload_count < conf.max_upload) {
				for (i = 0; i < conf.max_upload; i ++) {
					if (ul_pool[i].sleeping == 1) {
						if (pthread_mutex_lock(&ul_pool[i].th_mutex) != 0) {
							printf("ERRORE\n");
							continue;
						}
						ul_pool[i].ulnode.socksd = sock;
						ul_pool[i].sleeping = 0;
						pthread_cond_signal(&ul_pool[i].th_cond);
						if (pthread_mutex_unlock(&ul_pool[i].th_mutex) < 0) {
							printf("ERRORE\n");
							continue;
						}

						upload_count ++;
						printf("UPLOADS: %d\n", upload_count);
						break;
					}
				}

				if (i == conf.max_upload) {
					fprintf(stderr, "error - no uploader thread found\n");
					if (close(sock) < 0) {
						perror("close failed");
						return 1;
					}

				}

			} else {
				if (close(sock) < 0) {
					perror("close failed");
					return 1;
				}
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
					//	return 1;
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

