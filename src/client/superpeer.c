#include "superpeer.h"

/*
 * Funzione prova a connetersi ai superpeer indicati nella lista passata come parametro.
 * Ritorna 0 in caso di successo e -1 se si è verificato un errore o se non è riuscita
 * a connettersi a nessun superpeer della lista.
 */
int join_overlay(const struct sockaddr_in *sp_addr_list, int list_len) {
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
			fd_add(tcp_sock[free_sock]);
			free_sock ++;
		}
	}

	if (!ok) {
		close(tcp_sock[free_sock]);
	}

	if (free_sock == 0) {
		return -1;
	}

	printf("ESCO DA JOIN_OVERLAY\n");
	return 0;
	
}

/*
 * Funzione di inizializzazione del superpeer.
 */
int init_superpeer(int socksd, const struct sockaddr_in *sp_addr_list, int list_len) {

	printf("INIT SUPER PEER\n");
	is_sp = 0;
	free_sock = 0;
	curr_p_count = 0;
	have_child = 0; 
	curr_child_redirect = 0; 
	//inizializzo socket di ascolto
	if (set_listen_socket(get_local_port(socksd)) < 0) {
		fprintf(stderr, "init_superpeer error - set_listen_socket failed\n");
		return -1;
	}

	if (sp_addr_list != NULL) {
		//provo a connettermi agli altri superpeer
		if (join_overlay(sp_addr_list, list_len) < 0) {
			fprintf(stderr, "init_superpeer error - join_overlay failed\n");
			return -1;
		}
	}

	is_sp = 1;

	printf("ESCO DA INIT_SUPERPEER\n");
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
	} else {
		//TODO per essere sicuri che il peer è diventato superpeer dovremmo aspettare
		//un ack e poi settare l'indirizzo del figlio
		addrcpy(&child_addr, best_peer_addr);
		have_child = 1;
		return 0;
	}

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
	printf("aggiunto peer %s:%d\nrate: %ld\n", inet_ntoa(peer_addr->sin_addr), peer_addr->sin_port, peer_rate);
	printf("dimensione lista peer: %d\n", get_list_count(peer_list_head));
	curr_p_count ++;

	return 0;
}


