#include "peer_list_checker.h"

/*
 * Funzione che lancia il thread che controlla la lista di peer.
 * Ritorna 0 in caso di successo e -1 in caso di errore.
 */
int peer_list_checker_run(struct peer_list_ch_info *plchinfo) {
	int rc;

	if ((rc = pthread_mutex_init(&plchinfo->request_mutex, NULL)) != 0) {
		fprintf(stderr, "peer_list_checker_run error - can't initialize lock: %s\n", strerror(rc));
		return -1;
	}

	return thread_run(&plchinfo->thinfo, (void *)&peer_list_checker_func, (void *)plchinfo);
}

/*
 * Funzione che ferma il thread che controlla la lista di peer.
 * Ritorna 0 in caso di successo e -1 in caso di errore.
 */
int peer_list_checker_stop(struct peer_list_ch_info *plchinfo) {
	return thread_stop(&plchinfo->thinfo);
}

/*
 * Funzione che esegue il controllo della lista di peer.
 * In caso di errore il thread termina con valore di ritorno -1, altrimenti, se il thread
 * è interrotto e conclude correttamente il valore di ritorno è 0.
 */
void peer_list_checker_func(void *args) {
	struct peer_list_ch_info *plchinfo = (struct peer_list_ch_info *)args;
	struct timespec sleep_time;
	struct node *tmp_node;
	struct node *to_remove;
	struct request_node *req_it;
	int rc;

	printf("AVVIO PROCESSO DI CONTROLLO LISTA P\n");

	sleep_time.tv_sec = PL_CHECK_TIME;
	sleep_time.tv_nsec = 0;

	while (1) {
		nanosleep(&sleep_time, NULL);

		if ((rc = pthread_mutex_lock(&plchinfo->thinfo.mutex)) != 0) {
			fprintf(stderr, "peer_list_checker_func error - can't acquire lock: %s\n", strerror(rc));
			pthread_exit((void *)-1);
		}

		if (plchinfo->thinfo.go == 0) {
			if ((rc = pthread_mutex_unlock(&plchinfo->thinfo.mutex)) != 0) {
				fprintf(stderr, "peer_list_checker_func error - can't release lock: %s\n", strerror(rc));
				pthread_exit((void *)-1);
			}
			pthread_exit((void *)0);
		}
		
		//peer_list punta alla variabile che contiene l'indirizzo della testa della lista
		//quindi in tmp_node c'è il puntatore alla testa della lista.
		tmp_node = *plchinfo->peer_list;

		while (tmp_node != NULL) {
			to_remove = tmp_node;
			tmp_node = tmp_node->next;
			if (((struct peer_node *)to_remove->data)->flag == 0) {
				remove_all_file(((struct peer_node *)to_remove->data)->peer_addr.sin_addr.s_addr,
						((struct peer_node *)to_remove->data)->dw_port);
				remove_peer_node(to_remove);
				printf("RIMOSSO PEER NON ATTIVO\n");
			} else {
				((struct peer_node *)to_remove->data)->flag = 0;			
			}

		}

		if ((rc = pthread_mutex_unlock(&plchinfo->thinfo.mutex)) != 0) {
			fprintf(stderr, "peer_list_checker_func error - can't release lock: %s\n", strerror(rc));
			pthread_exit((void *)-1);
		}

		if ((rc = pthread_mutex_lock(&plchinfo->request_mutex)) != 0) {
			fprintf(stderr, "peer_list_checker_func error - can't acquire lock: %s\n", strerror(rc));
			pthread_exit((void *)-1);
		}
		req_it = request_fifo_tail;

		while (req_it != NULL) {
			if (req_it->ttl == 0) {
				remove_cascade_request(req_it);
				break;
			} else {
				req_it->ttl --;
			}
			req_it = req_it->next;
		}
		if ((rc = pthread_mutex_unlock(&plchinfo->request_mutex)) != 0) {
			fprintf(stderr, "peer_list_checker_func error - can't release lock: %s\n", strerror(rc));
			pthread_exit((void *)-1);
		}

	}

}

/*
 * Funzione che aggiorna il flag di presenza per il peer il cui indirizzo è
 * passato come parametro.
 * Ritorna 0 in caso di successo e -1 in caso di errore.
 */
int update_peer_flag(struct peer_list_ch_info *plchinfo, const struct sockaddr_in *peer_addr) {
	struct node *tmp_node;
	int rc;

	if ((rc = pthread_mutex_lock(&plchinfo->thinfo.mutex)) != 0) {
		fprintf(stderr, "update_peer_flag error - can't acquire lock: %s\n", strerror(rc));
		return -1;
	}

	tmp_node = *plchinfo->peer_list;

	while (tmp_node != NULL) {
		if (addrcmp(&((struct peer_node *)tmp_node->data)->peer_addr, peer_addr)) {
			((struct peer_node *)tmp_node->data)->flag = 1;
			break;
		}
		tmp_node = tmp_node->next;
	}

	if ((rc = pthread_mutex_unlock(&plchinfo->thinfo.mutex)) != 0) {
		fprintf(stderr, "update_peer_flag error - can't release lock: %s\n", strerror(rc));
		return -1;
	}

	return 0;
}

