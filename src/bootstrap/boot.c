#include "boot.h"

/*
 * Funzione che gestisce il comando ping. Questo comando viene inviato dai
 * superpeer per notificare la loro presenza. Il flag di presenza relativo
 * al superpeer viene agigornato.
 * Ritorna 0 in caso di successo, e -1 in caso di errore.
 */
int ping(const struct sockaddr_in *addr) {
	int lockfd;
	
	if ((lockfd = lock(LOCK_FILE)) < 0) {
		return -1;
	}

	if (set_sp_flag(addr, 1) == -1){
		fprintf(stderr, "errore ping\n");
	}
	
	if(unlock(LOCK_FILE, lockfd) < 0){
		return -1;
	}
	
	return 0;	
}

/*
 * Funzione che gestisce il comando register. Questo comando viene inviato da un peer
 * per informare il bootstrap che è diventato superpeer. L'indirizzo del peer viene
 * aggiunto alla lista di superpeer.
 * Ritorna 0 in caso di successo, e -1 in caso di errore.
 */
int sp_register(int sockfd, const struct sockaddr_in *addr, const struct packet *pck) {
	struct packet send_pack;
	int lockfd;
	
	if ((lockfd = lock(LOCK_FILE)) < 0) {
		return -1;
	}
	
	insert_sp(addr); 	
	
	if (send_addr_list(sockfd, addr, pck) < 0) {
		perror("errore invio lista IP");
		//RITORNO ????
	}
	
	if (unlock(LOCK_FILE, lockfd) < 0) {
		return -1;
	}
			
	new_ack_packet(&send_pack, pck->index);
	
	if (send_packet(sockfd, addr, &send_pack) < 0) {
		perror("errore in sendto");
		return -1;
	}
	
	return 0;
}

/*
 * Funzione che gestisce il comando leave. Questo comando è inviato dal superpeer
 * quando vuole abbandonare la rete overlay. L'indirizzo del superpeer viene eliminato
 * dalla lista degli indirizzi.
 * Ritorna 0 in caso di successo, e -1 in caso di errore.
 */
int sp_leave(int sockfd, const struct sockaddr_in *addr, const struct packet *pck) {
	int lockfd;
	struct packet ack_packet;
	
	if((lockfd = lock(LOCK_FILE)) < 0){
		return -1;
	}
	
	remove_sp(addr);   	
	
	if(unlock(LOCK_FILE, lockfd) < 0){
		return -1;
	}
	
	new_ack_packet(&ack_packet, pck->index);
	if (send_packet(sockfd, addr, &ack_packet) < 0) {
		perror("errore in sendto");
		return -1;
	}
	
	return 0;
}

int set_str_addrlist(char *str) {
	char *ptr;
	int i, ret;
	struct sockaddr_in *saddr;

	ptr = str;
	//di default invio 4 indirizzi
	ret = ADDR_TOSEND;
	if (sp_count < ADDR_TOSEND) {
		//ci sono meno di 4 indirizzi nella lista, invio tutti quelli che ci sono
		ret = sp_count;
	}

	for (i = 0; i < ret; i ++) {
		saddr = get_addr();
		addr2str(str, saddr->sin_addr.s_addr, saddr->sin_port);
		ptr += ADDR_STR_LEN;
	}

	return ret;
}

int send_addr_list(int sockfd, const struct sockaddr_in *addr, const struct packet *pck) {
	char str_to_send[ADDR_STR_LEN * ADDR_TOSEND]; 
	int dim;
	struct packet send_list; 
	
	//preparo il dato da inviare	
	dim = set_str_addrlist(str_to_send);  
	new_packet(&send_list, CMD_LIST, pck->index, str_to_send, dim * ADDR_STR_LEN, 1);
	if (send_packet(sockfd, addr, &send_list) < 0) {
		return -1;
	}
	
	return 0;
}

int set_sp_flag(const struct sockaddr_in *addr, short flag_value) {
	 struct spaddr_node *sp_node;
	 struct node *tmp_node;

	 tmp_node = get_node_sp(addr);

	 if (tmp_node == NULL) {
		printf("Indirizzo non trovato\n");
		return -1;
	 }
	 
	 sp_node = (struct spaddr_node *)tmp_node->data;
	 
	 if (sp_node == NULL) {
		printf("Errore in set_sp_flag: data è NULL\n");
		return -1;	 
	 }
	 
	 sp_node->flag = flag_value;
	 
	return 0;
}

int join(int sockfd, const struct sockaddr_in *addr, const struct packet *pck) {
	int lockfd;
	struct packet promote;
	
	if ((lockfd = lock(LOCK_FILE)) < 0) {
		return -1;
	}
	
	if (sp_list_head == NULL) {
		if(unlock(LOCK_FILE, lockfd) < 0) {
			return -1;
		}
		printf("lista vuota inviato promote\n");
		new_promote_packet(&promote, pck->index);
		if (send_packet(sockfd, addr, &promote) < 0) {
			perror("errore in sendto");
		}
		if ((lockfd = lock(LOCK_FILE)) < 0) {
			return -1;
		}
		insert_sp(addr);
		if(unlock(LOCK_FILE, lockfd) < 0) {
			return -1;
		}	
	} else {
		if(unlock(LOCK_FILE, lockfd) < 0) {
			return -1;
		}	
		if (send_addr_list(sockfd, addr, pck) < 0) {
			perror("errore invio lista IP");
		}
	}
	
	return 0;
}

int check_flag(void *unused) {
	struct node *tmp_node, *it;
	struct spaddr_node *spaddr_tmp;
	int lockfd;	
	
	while (1) {
		sleep(TIME_CHECK_FLAG);
		if ((lockfd = lock(LOCK_FILE)) < 0) {
			exit(1);
		}
		
		it = sp_list_head;
		while (it != NULL) {
			spaddr_tmp = (struct spaddr_node *)it->data;
			tmp_node = it;
			it = it->next;
			if (spaddr_tmp->flag == 0) {
				printf("cancello nodo: %lu:%u\n", (unsigned long)spaddr_tmp->sp_addr.sin_addr.s_addr, spaddr_tmp->sp_addr.sin_port);
				remove_sp_node(tmp_node);
			} else {
				spaddr_tmp->flag = 0;
				printf("Imposto flag: 0 per %lu:%u\n", (unsigned long)spaddr_tmp->sp_addr.sin_addr.s_addr, spaddr_tmp->sp_addr.sin_port);
			}
		}
		
		if (unlock(LOCK_FILE, lockfd) < 0) {
			exit(1);
		} 
	}
	
	return 0;

}


int main(int argc, char **argv) {
	int sockfd;
  	int stack_p[1024];
  	int len;
  	struct sockaddr_in addr;
// 	char buff[MAX_PACKET_SIZE];
  	pid_t pid;
  	char str[15];
	int n;
	struct packet recv_pck;
 	char strcmd[4];

    if ((sockfd = udp_socket()) < 0) {
        perror("socket error");
        return 1;
    }
    
    set_addr_any(&addr, SERV_PORT);  
    if (inet_bind(sockfd, &addr) < 0) {
        perror("bind error");
        return 1;
     }

	//elimino file lock se esiste
	unlink(LOCK_FILE);
	pid = clone((int (*)(void *))check_flag, &stack_p[1024], CLONE_VM, 0);
	
	len = sizeof(struct sockaddr_in);
	
	while (1) {
		if ((n = recvfrom_packet(sockfd, &addr, &recv_pck, &len)) < 0) {
			perror("errore in recvfrom");
			return 1;
		}
		strncpy(strcmd, recv_pck.cmd, CMD_STR_LEN);
		inet_ntop(AF_INET, &addr.sin_addr, str, 15);
		printf("ricevuto pachetto\n");
		printf("from %s:%d\n", str,ntohs(addr.sin_port));
		printf("data:%s\n", recv_pck.data);
		printf("cmd:%s\n", strcmd);

		if (!strncmp(recv_pck.cmd, CMD_PING, CMD_STR_LEN)) {
			ping(&addr);
		} else if (!strncmp(recv_pck.cmd, CMD_JOIN, CMD_STR_LEN)) {
			if (join(sockfd, &addr, &recv_pck) < 0) {
				fprintf(stderr, "join error\n");
			}
		} else if(!strncmp(recv_pck.cmd, CMD_LEAVE, CMD_STR_LEN)) {
			//delete sp
			if (sp_leave(sockfd, &addr, &recv_pck) < 0) {
				fprintf(stderr, "leave error\n");
			}
		} else if (!strncmp(recv_pck.cmd,CMD_REGISTER, CMD_STR_LEN)) {
			//insert sp
			if (sp_register(sockfd, &addr, &recv_pck) < 0) {
				fprintf(stderr, "register error\n");
			}			
		}
	}

	return 0;
}

