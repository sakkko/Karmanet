#include "superpeer.h"

/*
 * Funzione prova a connetersi ai superpeer indicati nella lista passata come parametro.
 * Ritorna 0 in caso di successo e -1 se si è verificato un errore o se non è riuscita
 * a connettersi a nessun superpeer della lista.
 */
int join_overlay(const struct sockaddr_in *sp_addr_list, int list_len) {
	int i, ok = 1;
	int sock_tmp;
	int addr_check = 0;
	int j;
	struct sockaddr_in *addr_list;

	memset(near_str, 0, MAX_TCP_SOCKET * 6);

	for (i = 0; i < list_len; i ++) {
		if (ok) {
			if ((sock_tmp = tcp_socket()) < 0) {
				perror("join_overlay error - can't initialize tcp socket");
				return -1;
			}
			printf("SOCKET: %d\n", sock_tmp);
			printf("LISTLEN: %d\n", list_len);
		}
		printf("join_overlay - addr: %s:%d\n", inet_ntoa(sp_addr_list[i].sin_addr), ntohs(sp_addr_list[i].sin_port));
		if (tcp_connect(sock_tmp, &sp_addr_list[i]) < 0) {
			perror("join_overlay error - can't connect to superpeer");
			ok = 0;
			continue; //provo il prossimo indirizzo
		} else {
			printf("Connected with superpeer %s:%d\n", inet_ntoa(sp_addr_list[i].sin_addr), ntohs(sp_addr_list[i].sin_port));
			if (addr_check == 0) {
				get_local_addr(sock_tmp, &myaddr);
				addr_check = 1;
			}
			ok = 1;
			fd_add(sock_tmp);

			if (write(sock_tmp, (char *)&conf.udp_port, sizeof(conf.udp_port)) < 0) {
				perror("join_overlay error - write failed\n");
				return -1;
			}

			if (insert_near(sock_tmp, &sp_addr_list[i]) < 0) {
				fprintf(stderr, "join_overlay error - insert_near failed\n");
				return -1;
			}

			addr2str(near_str + nsock * 6, sp_addr_list[i].sin_addr.s_addr, sp_addr_list[i].sin_port);	
			addr_list = str_to_addr(near_str, MAX_TCP_SOCKET);
			for(j = 0; j < MAX_TCP_SOCKET; j ++){
				printf("join_overlay - near %s:%d\n", inet_ntoa(addr_list[j].sin_addr), ntohs(addr_list[j].sin_port));
			}

			nsock ++;
		}
	}


	if (!ok) {
		close(sock_tmp);
	}

	if (nsock == 0) {
		return -1;
	}

	return 0;
	
}

/*
 * Funzione di inizializzazione del superpeer.
 */
int init_superpeer(const struct sockaddr_in *sp_addr_list, int list_len) {
	int rc;

	is_sp = 0;
	nsock = 0;
	curr_p_count = 0;
	have_child = 0; 
	curr_child_redirect = 0; 

	//inizializzo socket di ascolto
	if (set_listen_socket(conf.udp_port) < 0) {
		fprintf(stderr, "init_superpeer error - set_listen_socket failed\n");
		return -1;
	}

	if ((rc = pthread_mutex_init(&NEAR_LIST_LOCK, NULL)) != 0) {
		fprintf(stderr, "init_superpeer error - can't initialize lock: %s\n", strerror(rc));
		return -1;
	}

	if (sp_addr_list != NULL) {
		//provo a connettermi agli altri superpeer
		if (join_overlay(sp_addr_list, list_len) < 0) {
			fprintf(stderr, "init_superpeer error - join_overlay failed\n");
			return -1;
		}
		myaddr.sin_port = conf.udp_port;
		printf("MIO INDIRIZZO: %s:%d\n", inet_ntoa(myaddr.sin_addr), ntohs(myaddr.sin_port));
	}




	return 0;
	
}

/*
 * Funzione che inizializza la socket di ascolto tcp.
 * Ritorna 0 in caso successo e -1 in caso di errore.
 */
 
int set_listen_socket(unsigned short port) {
	struct sockaddr_in my_addr;

	if ((tcp_listen = tcp_socket()) < 0) {
		perror("set_listen_socket error - can't initialize tcp socket");
		return -1;
	}
	
	printf("PORT: %d\n", ntohs(port));
	set_addr_any(&my_addr, ntohs(port));
	if (inet_bind(tcp_listen, &my_addr)){
		perror ("set_listen_socket error - bind failed on tcp socket");
		return -1;
	}
	
	if (listen(tcp_listen, BACKLOG) < 0) {
		perror ("set_listen_socket error - listen failed on tcp socket");
		return -1;
	}

	fd_add(tcp_listen);

	return 0;
}

int promote_peer(int udp_sock, struct peer_list_ch_info *plchinfo) {
	struct packet tmp_packet;
	struct sockaddr_in *best_peer_addr;
	int rc;

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
	if (retx_send(udp_sock, best_peer_addr, &tmp_packet) < 0) {
		fprintf(stderr, "join_peer error: retx_send failed\n");
		return -1;
	} 		
	
	return 0;

}

int join_peer(const struct sockaddr_in *peer_addr, unsigned long peer_rate, struct peer_list_ch_info *plchinfo) {
	int rc;

	if ((rc = pthread_mutex_lock(&plchinfo->thinfo.mutex)) != 0) {
		fprintf(stderr, "join_peer error - can't acquire lock: %s\n", strerror(rc));
		return -1;
	}
	sorted_insert_peer(peer_addr, peer_rate);
	if ((rc = pthread_mutex_unlock(&plchinfo->thinfo.mutex)) != 0) {
		fprintf(stderr, "join_peer error - can't release lock: %s\n", strerror(rc));
		return -1;
	}
	printf("aggiunto peer %s:%d - rate: %ld\n", inet_ntoa(peer_addr->sin_addr), ntohs(peer_addr->sin_port), peer_rate);
	printf("dimensione lista peer: %d\n", get_list_count(peer_list_head));

	return 0;
}

int add_files(const struct sockaddr_in *peer_addr, const char *pck_data, int data_len) {
	int i, j;
	char tmp[1024];
	unsigned char md5[MD5_DIGEST_LENGTH];
	char op;

	j = 0;
	for (i = 0; i < data_len; i ++) {
		if (pck_data[i] != '+' && pck_data[i] != '-') {
			fprintf(stderr, "add_files error - bad packet format\n");
			return -1;
		}
		op = pck_data[i];
		memcpy(md5, pck_data + i + 1, MD5_DIGEST_LENGTH);
		
		for (i += MD5_DIGEST_LENGTH + 1 ; i < data_len; i ++) {
			if (pck_data[i] != '\n') {
				tmp[j] = pck_data[i];
				j ++;
			} else {
				tmp[j] = 0;
				j = 0;
				if (op == '+') {
					insert_file(tmp, md5, peer_addr->sin_addr.s_addr, peer_addr->sin_port);	
				} else {
					remove_file(peer_addr->sin_addr.s_addr, peer_addr->sin_port, tmp, md5);
				}

				break;
			}
		}
	}

	return 0;
}

int accept_conn(int tcp_listen) {
	int j, rc;
	int sock_tmp;
	int nread;
	unsigned short port;
	struct sockaddr_in *addr_list;
	struct sockaddr_in addr;
	unsigned int len = sizeof(struct sockaddr_in);

	if (nsock < MAX_TCP_SOCKET) {
		if ((sock_tmp = accept(tcp_listen, (struct sockaddr*)&addr, &len)) < 0) {
			perror("accept_conn error - accept failed");
			return -1;
		}
		if ((nread = read(sock_tmp, (char *)&port, sizeof(port))) != sizeof(port)) {
			if (nread < 0) {
				perror("accept_conn error - read failed");
			} else {
				fprintf(stderr, "accept_conn error - bad packet format\n");
			}
			return -1;
		}
		printf("connessione accettata da %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

		fd_add(sock_tmp);
		addr.sin_port = port;

		if ((rc = pthread_mutex_lock(&NEAR_LIST_LOCK)) != 0) {
			fprintf(stderr, "accept_conn error - can't acquire lock: %s\n", strerror(rc));
			return -1;
		}
		if (insert_near(sock_tmp, &addr) < 0) {
			fprintf(stderr, "join_overlay error - insert_near failed\n");
			return -1;
		}

		set_near();
		printf("accept_conn setto la stringa dei vicini\n");
		if ((rc = pthread_mutex_unlock(&NEAR_LIST_LOCK)) != 0) {
			fprintf(stderr, "accept_conn error - can't release lock: %s\n", strerror(rc));
			return -1;
		}
	//	printf("connessione accettata da %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
		addr_list = str_to_addr(near_str, MAX_TCP_SOCKET);
		for(j = 0 ; j < MAX_TCP_SOCKET; j ++){
			printf("accept - near %s:%d\n", inet_ntoa(addr_list[j].sin_addr), ntohs(addr_list[j].sin_port));
		}
		nsock ++;
	} else {
		fprintf(stderr, "accept_conn error - can't accept more connection\n");
		return -1;
	}

	return 0;
}

int close_conn(int sock) {
	
	if (close(sock) < 0) {
		perror("close_conn error - close failed");
		return -1;
	}

	fd_remove(sock);

	if (delete_near(sock) < 0) {
		fprintf(stderr, "close_conn error - socket not found in near list\n");
		return -1;
	}

	set_near();
	nsock --;

	return 0;

}

void set_near() {
	struct near_node *iterator = near_list_head;
	memset(near_str, 0, MAX_TCP_SOCKET * 6);
	int n = 0;

	while (iterator != NULL) {
		addr2str(near_str + n * 6, iterator->addr.sin_addr.s_addr, iterator->addr.sin_port);	
		iterator = iterator->next;
		n ++;
	}
}


int flood_overlay(const struct packet *pck, int socksd) {
	struct near_node *iterator = near_list_head;
	struct near_node *near_info;
	int rc;

	near_info = get_near_node(socksd);

	while (iterator != NULL) {		
		if (iterator->socksd == socksd) {
			iterator = iterator->next;
			continue;
		}
		
		if (near_info != NULL && contains_addr(near_info->data, near_info->near_number * ADDR_STR_LEN, &iterator->addr)) {
			iterator = iterator->next;
			continue;
		}

		if ((rc = pthread_mutex_lock(&iterator->mutex)) != 0) {
			fprintf(stderr, "flood_overlay error - can't acquire lock: %s\n", strerror(rc));
			return -1;
		}
		if (send_packet_tcp(iterator->socksd, pck) < 0) {
			fprintf(stderr, "flood_overlay error - send_packet_tcp failed\n");
			//return -1;
		}
		if ((rc = pthread_mutex_unlock(&iterator->mutex)) != 0) {
			fprintf(stderr, "flood_overlay error - can't release lock: %s\n", strerror(rc));
			return -1;
		}

		iterator = iterator->next;
	}

	return 0;
}

