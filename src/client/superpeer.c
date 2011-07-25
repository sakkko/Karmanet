#include "superpeer.h"

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
	is_sp = 0;
	free_sock = 0;
	curr_p_count = 0;
	have_child = 0; 
	curr_child_redirect = 0; 
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

int join_peer(int udp_sock, const struct sockaddr_in *addr, const struct packet *pck, struct peer_list_ch_info *plchinfo) {
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
				if (retx_send(udp_sock, best_peer_addr, &tmp_packet) < 0) {
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

