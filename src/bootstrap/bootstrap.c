#include "bootstrap.h"

/*
 * Funzione che gestisce il comando register. Questo comando viene inviato da un peer
 * per informare il bootstrap che è diventato superpeer. L'indirizzo del peer viene
 * aggiunto alla lista di superpeer.
 * Ritorna 0 in caso di successo, e -1 in caso di errore.
 */
int sp_register(int sockfd, const struct sockaddr_in *addr, const struct packet *pck, struct sp_list_checker_info *splchinfo) {
	struct packet send_pack;
	int rc;

	printf("SP REGISTER\n");
	
	if (pck->data_len != 1) {
		fprintf(stderr, "sp_register error - invalid packet format\n");
		return -1;
	}

	if ((rc = pthread_mutex_lock(&splchinfo->thinfo.mutex)) != 0) {
		fprintf(stderr, "sp_register error - can't acquire lock: %s\n", strerror(rc));
		return -1;
	}

	insert_sp(addr, *pck->data); 	
	
	if ((rc = pthread_mutex_unlock(&splchinfo->thinfo.mutex)) != 0) {
		fprintf(stderr, "sp_register error - can't release lock: %s\n", strerror(rc));
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
int sp_leave(int sockfd, const struct sockaddr_in *addr, const struct packet *pck, struct sp_list_checker_info *splchinfo) {
	struct packet ack_packet;
	int rc;

	if ((rc = pthread_mutex_lock(&splchinfo->thinfo.mutex)) != 0) {
		fprintf(stderr, "sp_leave error - can't acquire lock: %s\n", strerror(rc));
		return -1;
	}
	
	remove_sp(addr);   	
	
	if ((rc = pthread_mutex_unlock(&splchinfo->thinfo.mutex)) != 0) {
		fprintf(stderr, "sp_leave error - can't release lock: %s\n", strerror(rc));
		return -1;
	}
	
	new_ack_packet(&ack_packet, pck->index);
	if (send_packet(sockfd, addr, &ack_packet) < 0) {
		perror("errore in sendto");
		return -1;
	}
	
	return 0;
}

/*
 * Funzione che scrive gli indirizzi da iviare in byte nella stringa passata come parametro.
 * Ritorna il numero di indirizzo scritti nella stringa.
 */
int set_str_addrlist(char *str, int superpeer, struct sp_list_checker_info *splchinfo) {
	char *ptr;
	int i, rc, ret;
	struct sockaddr_in *saddr;
	struct node *it_tmp, *it_tmp2;

	if (superpeer == 0) {
		ptr = str;
		ret = addr_to_send;
		if (sp_count < addr_to_send) {
			//ci sono meno di 4 indirizzi nella lista, invio tutti quelli che ci sono
			ret = sp_count;
		}

		if ((rc = pthread_mutex_lock(&splchinfo->thinfo.mutex)) != 0) {
			fprintf(stderr, "set_str_addrlist error - can't acquire lock: %s\n", strerror(rc));
			return -1;
		}

		saddr = get_addr();
		it_tmp = it_addr;

		for (i = 0; i < ret; i ++) {
			addr2str(ptr, saddr->sin_addr.s_addr, saddr->sin_port);
			ptr += ADDR_STR_LEN;
			saddr = get_addr();
		}

		it_addr = it_tmp;

		if ((rc = pthread_mutex_unlock(&splchinfo->thinfo.mutex)) != 0) {
			fprintf(stderr, "set_str_addrlist error - can't release lock: %s\n", strerror(rc));
			return -1;
		}
	} else {
		ptr = str;
		if ((rc = pthread_mutex_lock(&splchinfo->thinfo.mutex)) != 0) {
			fprintf(stderr, "set_str_addrlist error - can't acquire lock: %s\n", strerror(rc));
			return -1;
		}

		it_tmp = get_addr_sp(&ret);

		if ((rc = pthread_mutex_unlock(&splchinfo->thinfo.mutex)) != 0) {
			fprintf(stderr, "set_str_addrlist error - can't release lock: %s\n", strerror(rc));
			return -1;
		}
		if (it_tmp == NULL) {
			fprintf(stderr, "set_str_addrlist error - no free super-peer found\n");
			return -1;
		}
		it_tmp2 = it_tmp;

		while (it_tmp2 != NULL) {
			addr2str(ptr, ((struct spaddr_node *)it_tmp2->data)->sp_addr.sin_addr.s_addr, ((struct spaddr_node *)it_tmp2->data)->sp_addr.sin_port);
			printf("INDIRIZZO SP: %s:%u\n", inet_ntoa(((struct spaddr_node *)it_tmp2->data)->sp_addr.sin_addr), ntohs(((struct spaddr_node *)it_tmp2->data)->sp_addr.sin_port));
			ptr += ADDR_STR_LEN;
			it_tmp2 = it_tmp2->next;
		}

		free_list(it_tmp);
		
	}
	return ret;
}

/*
 * Funzione che invia la lista degli indirizzi dei superpeer al client il cui indirizzo è
 * passato come parametro.
 * Ritorna 0 in caso di successo e -1 in caso di errore.
 */
int send_addr_list(int sockfd, const struct sockaddr_in *addr, const struct packet *pck, struct sp_list_checker_info *splchinfo) {
	char str_to_send[ADDR_STR_LEN * ADDR_TOSEND]; 
	int dim;
	struct packet send_list; 
	
	//preparo il dato da inviare	
	dim = set_str_addrlist(str_to_send, is_set_flag(pck, PACKET_FLAG_SUPERPEER), splchinfo);  
	if (dim < 0) {
		fprintf(stderr, "send_addr_list error - set_str_addrlist failed\n");
		return -1;
	}
	new_packet(&send_list, CMD_LIST, pck->index, str_to_send, dim * ADDR_STR_LEN, 1);
	if (send_packet(sockfd, addr, &send_list) < 0) {
		return -1;
	}
	
	return 0;
}

/*
 * Funzione che gestisce il comando JOIN. Questo comando è inviato da un client che
 * vuole entrare nella rete. Il bootstrap invia in risposta la lista degli indirizzi
 * dei superpeer, oppure PROMOTE se la lista è vuota e in tal caso salva l'indirizzo
 * del client nella lista dei superpeer.
 * Ritorna 0 in caso di successo e -1 in caso di errore.
 */
int sp_join(int sockfd, const struct sockaddr_in *addr, const struct packet *pck, struct sp_list_checker_info *splchinfo) {
	struct packet promote;
	int rc;

	if ((rc = pthread_mutex_lock(&splchinfo->thinfo.mutex)) != 0) {
		fprintf(stderr, "sp_join error - can't acquire lock: %s\n", strerror(rc));
		return -1;
	}
	
	if (sp_list_head == NULL) {
		printf("lista vuota inviato promote\n");
		insert_sp(addr, ADDR_TOSEND * 2);
		if ((rc = pthread_mutex_unlock(&splchinfo->thinfo.mutex)) != 0) {
			fprintf(stderr, "sp_join error - can't release lock: %s\n", strerror(rc));
			return -1;
		}

		new_promote_packet(&promote, pck->index);
		addr2str(promote.data, addr->sin_addr.s_addr, addr->sin_port);
		promote.data_len = 6;	
		if (send_packet(sockfd, addr, &promote) < 0) {
			perror("sp_join error - send_packet failed");
			return -1;
		}
	} else {
		if (sp_count == 1) {
			if (addrcmp(&((struct spaddr_node *)sp_list_head->data)->sp_addr, addr)) {
				if ((rc = pthread_mutex_unlock(&splchinfo->thinfo.mutex)) != 0) {
					fprintf(stderr, "sp_join error - can't release lock: %s\n", strerror(rc));
					return -1;
				}
				new_promote_packet(&promote, pck->index);
				addr2str(promote.data, addr->sin_addr.s_addr, addr->sin_port);
				promote.data_len = 6;	
				printf("INVIO PROMOTE\n");
				if (send_packet(sockfd, addr, &promote) < 0) {
					perror("sp_join error - send_packet failed");
					return -1;
				}
				return 0;
			} 
		}

		if ((rc = pthread_mutex_unlock(&splchinfo->thinfo.mutex)) != 0) {
			fprintf(stderr, "sp_join error - can't release lock: %s\n", strerror(rc));
			return -1;
		}
		if (send_addr_list(sockfd, addr, pck, splchinfo) < 0) {
			fprintf(stderr, "sp_join error - send_addr_list failed");
			return -1;
		}
	}
	
	return 0;
}

int main(int argc, char **argv) {
	int sockfd;
  	int len;
  	struct sockaddr_in addr;
	int n;
	struct packet recv_pck;

	struct sp_list_checker_info spl_check_info;

    if ((sockfd = udp_socket()) < 0) {
        perror("socket error");
        return 1;
    }
    
    set_addr_any(&addr, SERV_PORT);  
    if (inet_bind(sockfd, &addr) < 0) {
        perror("bind error");
        return 1;
     }

	addr_to_send = ADDR_TOSEND;

	//inizializzo struttura da passare al thread che controlla la lista dei superpeer
	spl_check_info.sp_list = &sp_list_head;
	//faccio partire il thread che controlla la lista di superpeer
	if (sp_list_checker_run(&spl_check_info) < 0) {
		fprintf(stderr, "Can't start superpeer list checker\n");
		return 1;
	}
	
	len = sizeof(struct sockaddr_in);
	
	while (1) {
		if ((n = recvfrom_packet(sockfd, &addr, &recv_pck, &len)) < 0) {
			perror("errore in recvfrom");
			return 1;
		}

		if (!strncmp(recv_pck.cmd, CMD_PING, CMD_STR_LEN)) {
			//aggiorno il flag del superpeer che mi ha pingato
			if (update_sp_flag(&spl_check_info, &addr, *recv_pck.data) < 0) {
				fprintf(stderr, "Ping error\n");
			}
		} else if (!strncmp(recv_pck.cmd, CMD_JOIN, CMD_STR_LEN)) {
			//join sp
			if (sp_join(sockfd, &addr, &recv_pck, &spl_check_info) < 0) {
				fprintf(stderr, "Join error\n");
			}
		} else if(!strncmp(recv_pck.cmd, CMD_LEAVE, CMD_STR_LEN)) {
			//delete sp
			if (sp_leave(sockfd, &addr, &recv_pck, &spl_check_info) < 0) {
				fprintf(stderr, "Leave error\n");
			}
		} else if (!strncmp(recv_pck.cmd, CMD_REGISTER, CMD_STR_LEN)) {
			//insert sp
			if (sp_register(sockfd, &addr, &recv_pck, &spl_check_info) < 0) {
				fprintf(stderr, "Register error\n");
			}			
		}
	}

	return 0;
}

