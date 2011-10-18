#include "superpeer.h"

/*
 * Funzione prova a connetersi ai superpeer indicati nella lista passata come parametro.
 * Ritorna 0 in caso di successo e -1 se si è verificato un errore o se non è riuscita
 * a connettersi a nessun superpeer della lista.
 */
int join_overlay(const struct sockaddr_in *sp_addr_list, int list_len) {
	int i, ok = 1;
	int addr_check = 0;
	int rc;
	for (i = 0;  i < list_len; i ++) {
		if (ok) {
			if ((tcp_sock[free_sock] = tcp_socket()) < 0) {
				perror("join_overlay error - can't initialize tcp socket");
				return -1;
			}
		}
		printf("join_overlay - addr: %s:%d\n", inet_ntoa(sp_addr_list[i].sin_addr), ntohs(sp_addr_list[i].sin_port));
		if (tcp_connect(tcp_sock[free_sock], &sp_addr_list[i]) < 0){
			perror("join_overlay error - can't connect to superpeer");
			ok = 0;
			continue; //provo il prossimo indirizzo
		} else {
			printf("Connected with superpeer %s:%d\n", inet_ntoa(sp_addr_list[i].sin_addr), ntohs(sp_addr_list[i].sin_port));
			if (addr_check == 0) {
				get_local_addr(tcp_sock[free_sock], &myaddr);
				addr_check = 1;
			}
			ok = 1;
			fd_add(tcp_sock[free_sock]);
			if((rc = pthread_mutex_init(&tcp_lock[free_sock],NULL))!=0){
				fprintf(stderr,"join_overlay error - can't initialize lock: %s\n",strerror(rc));
				return -1;
				}
			addr2str(near_str+free_sock*6,sp_addr_list[i].sin_addr.s_addr, sp_addr_list[i].sin_port);	
			struct sockaddr_in *addr_list;
				addr_list = str_to_addr(near_str,MAX_TCP_SOCKET);
				int j;
				for(j = 0 ; j< MAX_TCP_SOCKET; j++){
					printf("join_overlay - near %s:%d\n",inet_ntoa(addr_list[j].sin_addr),ntohs(addr_list[j].sin_port));
				}
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
int init_superpeer(const struct sockaddr_in *sp_addr_list, int list_len) {
	int i;

	is_sp = 0;
	free_sock = 0;
	curr_p_count = 0;
	have_child = 0; 
	curr_child_redirect = 0; 
	//inizializzo socket di ascolto
	if (set_listen_socket(conf.udp_port) < 0) {
		fprintf(stderr, "init_superpeer error - set_listen_socket failed\n");
		return -1;
	}

	for (i = 0; i < MAX_TCP_SOCKET; i ++) {
		tcp_sock[i] = -1;
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
	int i;
	int rc;
	struct sockaddr_in addr;
	int len= sizeof(struct sockaddr_in);
	if (free_sock < MAX_TCP_SOCKET) {
		for (i = 0; i < MAX_TCP_SOCKET; i ++) {
			if (tcp_sock[i] == -1) {
				if ((tcp_sock[i] = accept(tcp_listen, (struct sockaddr*)&addr, &len)) < 0) {
					perror("errore in accept");
					return -1;
				}
				fd_add(tcp_sock[i]);
				
				if((rc = pthread_mutex_init(&tcp_lock[i],NULL))!=0){
					fprintf(stderr,"accept_conn error - can't initialize lock: %s\n",strerror(rc));
					return -1;
				}
				addr2str(near_str+i*6,addr.sin_addr.s_addr, addr.sin_port);	
				printf("connessione accettata da %s:%d\n",inet_ntoa(addr.sin_addr),ntohs(addr.sin_port));
				struct sockaddr_in *addr_list;
				addr_list = str_to_addr(near_str,MAX_TCP_SOCKET);
				int j;
				for(j = 0 ; j< MAX_TCP_SOCKET; j++){
					printf("accept - near %s:%d\n",inet_ntoa(addr_list[j].sin_addr),ntohs(addr_list[j].sin_port));
				}
				free_sock ++;
				break;
			}
		}

		if (i == MAX_TCP_SOCKET) {
			fprintf(stderr, "accept_conn error - no free socket found\n");
			return -1;
		}
	} else {
		fprintf(stderr, "accept_conn error - can't accept more connection\n");
		return -1;
	}

	return 0;
}

int close_conn(int sock) {
	int i;
	int rc;
	for (i = 0; i < MAX_TCP_SOCKET; i ++) {
		if (tcp_sock[i] == sock) {
			if (close(tcp_sock[i]) < 0) {
				perror("close_conn error - close failed");
				return -1;
			}
			fd_remove(tcp_sock[i]);
			tcp_sock[i] = -1;
			if((rc = pthread_mutex_destroy(&tcp_lock[i]))!=0){
				fprintf(stderr,"close_conn error - can't initialize lock: %s\n",strerror(rc));
				return -1;
			}
			memcpy(near_str+i*6, "\0" ,6);
			free_sock --;
			break;
		}
	}

	if (i == MAX_TCP_SOCKET) {
		printf("close_conn error - no socket found\n");
		return -1;
	}

	return 0;

}

