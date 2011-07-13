#include "client.h"

void add_fd(int fd, fd_set *fdset) {
	FD_SET(fd, fdset);
	if (fd > maxd) {
		maxd = fd;
	}	
}

int ping_func(void) {
	struct packet tmp_pck;
	while (1) {
		sleep(3);
		new_ping_packet(&tmp_pck, get_index());	
		if (is_sp == 1) {
			mutex_send(udp_sock, &bs_addr, &tmp_pck);	
		} else {
			mutex_send(udp_sock, &my_sp_addr, &tmp_pck);		
		}
	}	
}

/*
* Funzione che aggiorna il flag di peer_node ad 1
*/
void update_peer_flag(const struct sockaddr_in *peer_addr) {

	struct node *tmp_node;
	
	tmp_node = peer_list_head;
	int lockfd;	
	if ((lockfd = lock(LOCK_MY_P)) < 0) {
		exit(1);
	}		
	while (tmp_node != NULL) {
		if (addrcmp(&((struct peer_node *)tmp_node->data)->peer_addr, peer_addr)) {
			((struct peer_node *)tmp_node->data)->flag = 1;
			break;
		}
		tmp_node = tmp_node->next;
	}


	if (unlock(LOCK_MY_P, lockfd) < 0) {
		exit(1);
	}

	return;
}


/*
* Funzione che controlla i flag della lista elimina i nodi con flag 0 e setta a 0 quelli con flag 1
*/
void check_peer_flag(void* unused) {
	int lockfd;	
	int dim = 0;
	struct node *tmp_node;
	struct node *to_remove;
	printf("AVVIO PROCESSO DI CONTROLLO LISTA P\n");
	while(1){
	  sleep(10);	
		printf("wake up and starting work\n");
		if ((lockfd = lock(LOCK_MY_P)) < 0) {
			exit(1);
		}

		tmp_node = peer_list_head;
		dim = 0;
		while (tmp_node != NULL) {
		
			to_remove=tmp_node;
			tmp_node = tmp_node->next;
			if (((struct peer_node *)to_remove->data)->flag == 0) {
				//remove_peer(&((struct peer_node *)to_remove->data)->peer_addr);
				remove_peer_node(to_remove);
				printf("RIMOSSO PEER NON ATTIVO\n");
			}
			else{
				dim++;
				printf("TROVATO FLAG A 1\n");
				((struct peer_node *)to_remove->data)->flag=0;			
			}

		}

		if (unlock(LOCK_MY_P, lockfd) < 0) {
			exit(1);
		}
		
		printf("work done dim = %d\n",dim); 	
	}
}

int update_sp(){
	int lockfd2;	
	if ((lockfd2 = lock(LOCK_MY_SP)) < 0) {
		exit(1);
	}
	
	my_sp_flag = 1;
	
	if (unlock(LOCK_MY_SP, lockfd2) < 0) {
		exit(1);
	}

	return 0;
}

int check_sp(void* unused){
	int lockfd3;	
	printf("AVVIO PROCESSO DI CONTROLLO SP\n");
	while(1){
	  sleep(TIME_CHECK_FLAG);	
		if ((lockfd3 = lock(LOCK_MY_SP)) < 0) {
			exit(1);
		}
	
		if(my_sp_flag == 0){
			//rimuovere sp
			printf("IL SUPER PEER NON RISPONDE\n");
//			my_sp_addr.sin_addr.s_addr = 0;
													
		}
		else{
			my_sp_flag = 0;
		
		}
	
		if (unlock(LOCK_MY_SP, lockfd3) < 0) {
			exit(1);
		} 	
	}
	

}

short get_index() {
	short ret = start_index;
	start_index ++;
	return ret;
}

void init_index() {
	start_index = get_rand();
}

void print_addr_list(struct sockaddr_in *addr, int dim) {
	int i = 0;
	int off = sizeof(struct sockaddr_in);
	for (i = 0; i < dim; i ++) {
		printf("%d address: %lu : %u\n", i, (unsigned long)addr[off *i].sin_addr.s_addr, addr[off *i].sin_port);
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

	set_rate();
	init_index();
	retx_init();
	
	unlink(LOCK_MY_SP);
	unlink(LOCK_MY_P);
	return 0;
}

//processo di inizializzazione decido se sono peer o sp in caso restituisce la lista di ind
struct sockaddr_in *start_process_OLD(fd_set *fdset, int *list_len) {
	struct packet send_pck, recv_pck;
	struct sockaddr_in *addr_list;
		
	//invio join al boostrap
	new_join_packet(&send_pck, get_index()); 
	printf("invio join\n");
	
	if (retx_send(udp_sock, &bs_addr, &send_pck) < 0) {
		perror("errore in sendto");
		return NULL;	
	}
	
	printf("attendo risposta\n");

	if (retx_recv(udp_sock, &recv_pck) < 0) {
		perror("errore in recvfrom");
		return NULL;	
	}

	printf("ricevuti dati cmd:%s index:%d data_len:%d data:%s\n", recv_pck.cmd, recv_pck.index, recv_pck.data_len, recv_pck.data);
		
	add_fd(udp_sock, fdset);
	
	if (!strncmp(recv_pck.cmd, CMD_LIST, CMD_STR_LEN)) {
		//ricevo lista di sp
		// sono un peer
		addr_list = str_to_addr(recv_pck.data, recv_pck.data_len / 6);
		*list_len = recv_pck.data_len / 6;
		return addr_list;
	} else if (!strncmp(recv_pck.cmd, CMD_PROMOTE, CMD_STR_LEN)) {
		//ricevo promote e sono il primo sp
		printf("DIVENTO SUPER PEER\n");
		is_sp = 1;
	} else {
		printf("RICEVUTI DATI NON RICONOSCIUTI\n");
	}
	
	return NULL;
}

//inizializzo un sp imposto array descrittori,imposto il mio indirizzo,mi connetto a chi posso
int init_sp(fd_set *fdset, const struct sockaddr_in *addr, int  dim, struct sockaddr_in *my_addr) {
	int i;
	
	//devo inviare il register al bs
	
	if ((tcp_listen = tcp_socket()) < 0) {
		perror("errore in socket");
		return -1;
	}
	
	set_addr_any(my_addr, TCP_PORT);
	if (inet_bind(tcp_listen, my_addr)){
		perror ("errore in bind");
		return -1;
	}
	
	if (listen(tcp_listen, BACKLOG) < 0) {
		perror("errore in listen");
		return -1;
	}
	
	add_fd(tcp_listen, fdset);
	
	if (addr != NULL){
		for (i = 0;  i < dim; i ++) {
			tcp_sock[free_sock] = tcp_socket();
			if (tcp_connect(tcp_sock[free_sock], &addr[i]) < 0){
				printf("fallito tentaativo di connessione\n");
				close(tcp_sock[free_sock]);
				continue; //provo il prossimo indirizzo
			} else {
				add_fd(tcp_sock[free_sock], fdset);
				free_sock ++;
			}
		}
	}

	return 0;
}


//inizializzo un peer : iposto aarray descrittori (in caso), imposto il sp a me associato,imposto mio indirizzo ( se ricevo promote)
int init_peer(fd_set *allset, struct sockaddr_in  *addr, int list_len, struct sockaddr_in *my_addr) {
	int i, len;
	struct packet send_pck, recv_pck;

	new_join_packet_rate(&send_pck, get_index(), peer_rate);
	len = sizeof(struct sockaddr_in);
	
	for (i = 0; i < list_len; i ++) { 
		if (retx_send(udp_sock, &addr[i], &send_pck) < 0) {
			perror ("errore in udp_send");
		 	continue; //provo il prox indirizzo
		}
		if (retx_recvfrom(udp_sock, &my_sp_addr, &recv_pck, &len) < 0) {
			perror ("errore in udp_recv");
			continue; //prova ancora
		}
		if (!strncmp(recv_pck.cmd, CMD_ACK, CMD_STR_LEN)) {
			
			my_sp_flag = 1;
			pid_csp = clone((int(*)(void *))check_sp, &stack_csp[1024], CLONE_VM, 0); 
			
			//ricevuto ack
			//connessione accettata
			//invio la mia lista di file
			//TODO send_my_file () 
			return 0;
		} else if (!strcmp(recv_pck.cmd, CMD_PROMOTE)) { 
			//ricevuto promote
			new_ack_packet(&send_pck, recv_pck.index);
	 		if(mutex_send(udp_sock, &my_sp_addr, &send_pck) < 0){
	 			perror ("errore in udp_send");
	 			return -1;
	 		}	
	 		//uso la stessa lista che ho ricevuto prima	
	 		init_sp(allset,addr,list_len,my_addr); //divento sp
	 		 //TODO join (bs_addr); faccio il join al server di bs
	 		return 0;
	 	} else if (!strcmp(recv_pck.cmd, CMD_REDIRECT)) {
	 		//TODO leggo dal pacchetto l'indirizzo
	 		retx_stop(recv_pck.index);
	 		new_ack_packet(&send_pck, recv_pck.index);
	 		if (mutex_send(udp_sock, &my_sp_addr, &send_pck) < 0) {
	 			perror ("errore in udp_send");
	 			continue; //prova un altro indirizzo
	 		}
	 		//>TODO impostare la nuova lista di indirizzi aggiungendo l'indirizzo ricevuto nel redirect
	 		//ed eliminando tutti gli indirizzi controllati finora	
			//TODO init_peer (); //richiamo su indirizzo nel redirect
		}
	}
	
	return 0;
}

int join_peer(const struct sockaddr_in *addr, const struct packet *pck) {
	unsigned long peer_rate;
	struct packet tmp_packet;

	if (is_sp == 0) {
		new_err_packet(&tmp_packet, pck->index);
	} else {
		if (curr_p_count < MAX_P_COUNT) {
			//posso accettare il peer
			peer_rate = btol(pck->data);
			sorted_insert_peer(addr, peer_rate);
			new_ack_packet(&tmp_packet, pck->index);
			printf("aggiunto peer %s:%d\nrate: %ld\n", inet_ntoa(addr->sin_addr), addr->sin_port, peer_rate);
			printf("dimensione lista peer: %d\n", get_list_count(peer_list_head));
			insert_request(addr, pck->index);
		} else {
			if (have_child) {
				new_redirect_packet(&tmp_packet, pck->index, &child_addr);
			} else {
				new_promote_packet(&tmp_packet, get_index());
				if (retx_send(udp_sock, get_addr_head(), &tmp_packet) < 0) {
					fprintf(stderr, "join_peer error: retx_send failed\n");
					return -1;
				} else {
					//TODO per essere sicuri che il peer è diventato superpeer dovremmo aspettare
					//un ack e poi settare l'indirizzo del figlio
					addrcpy(&child_addr, get_addr_head());
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

void udp_handler(int udp_sock, fd_set *allset) {
	struct packet tmp_pck;
	struct sockaddr_in addr;
	int len = sizeof(struct sockaddr_in); 
	
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
			join_peer(&addr, &tmp_pck);
			insert_request(&addr,tmp_pck.index);
		} else if (!strncmp(tmp_pck.cmd, CMD_PING, CMD_STR_LEN)) { 
			//printf("ricevuto ping\n");
			//updateflag di addr
			update_peer_flag(&addr);
	 		new_pong_packet(&tmp_pck, tmp_pck.index);
		 	if (mutex_send(udp_sock, &addr, &tmp_pck) < 0) {
		 		perror ("errore in udp_send");
		 	}
		} else if (!strncmp(tmp_pck.cmd, CMD_LEAVE, CMD_STR_LEN)) { 
			//rimuovi addr
		} else if (!strncmp(tmp_pck.cmd, CMD_WHOHAS, CMD_STR_LEN)) { 
			//rimuovi addr
		}
	} else {
		//sono solo peer
		if (!strncmp(tmp_pck.cmd, CMD_PONG, CMD_STR_LEN)) { 
			//printf("ricevuto pong\n");
			//updateflag di addr
			update_sp();
		}
	}
}

int main (int argc,char *argv[]){
	struct sockaddr_in  *address; //indirizzi ricevuti dal bs
	struct sockaddr_in my_addr; //mio indirizzo (quando sono sp)
	int list_len = 0, i; //lunghezza indirizzi ricevuti
	int ready;
//	int dif_fd; //descrittore del file differenze
//	char buf[1024];
	fd_set rset, allset;
	pid_t pid,pid_cp;
	long stack_p[1024],stack_cp[1024];

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
	
	//dif_fd = open ("difference.txt",O_WRONLY|O_CREAT, 0666);
	
	//provo a entrare nella rete
	address = start_process_OLD(&allset, &list_len);
	printf("list_len %d\n", list_len);

	if (is_sp == 1) {	
		//sono superpeer
		init_sp(&allset, address, list_len, &my_addr);
		pid_cp = clone((int(*)(void *))check_peer_flag, &stack_cp[1024], CLONE_VM, 0);  
	} else {
		if (address == NULL) {
			fprintf(stderr, "Errore durante la connessione\n");
			return 1;
		}
		//sono peer mi connetto a un sp della lista
		print_addr_list(address,list_len);	
		init_peer(&allset, address, list_len, &my_addr);
	}
	
	pid = clone((int(*)(void *))ping_func, &stack_p[1024], CLONE_VM, 0);

	//main loop - select	
	while (1) {
		rset = allset;
		if ((ready = select(maxd + 1, &rset, NULL, NULL, NULL)) < 0) {
      		perror("errore in select");
      		exit(1);
    	}
    	
    	if (FD_ISSET(udp_sock, &rset)){ 
    		 udp_handler (udp_sock , &allset);
    	} 
    	else if (FD_ISSET(fileno(stdin), &rset)){
    		//TODO keybord_handler ();
			;
/*    	} else if (FD_ISSET(dif_fd, &rset)) {
    		printf("descrittore diff attivo\n");
		    //se ho modificato i file condivisi
    		//TODO	dif_handler ();
*/
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

