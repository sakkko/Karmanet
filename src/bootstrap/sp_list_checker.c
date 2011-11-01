#include "sp_list_checker.h"

/*
 * Funzione che lancia il thread che controlla la lista dei superpeer.
 * Ritorna 0 in caso di successo e -1 in caso di errore.
 */
int sp_list_checker_run(struct sp_list_checker_info *splchinfo) {
	return thread_run(&splchinfo->thinfo, (void *)&sp_list_checker_func, (void *)splchinfo);
}

/*
 * Funzione che ferma il thread che controlla la lista dei superpeer.
 * Ritorna 0 in caso di successo e -1 in caso di errore.
 */
int sp_list_checker_stop(struct sp_list_checker_info *splchinfo) {
	return thread_stop(&splchinfo->thinfo);
}

/*
 * Funzione che esegue il controllo della lista dei superpeer.
 * In caso di errore il thread termina con valore di ritorno -1, altrimenti, se il thread
 * è interrotto e conclude correttamente il valore di ritorno è 0.
 */
void sp_list_checker_func(void *args) {
	struct sp_list_checker_info *splchinfo = (struct sp_list_checker_info *)args;
	struct node *tmp_node, *it;
	struct spaddr_node *spaddr_tmp;
	struct timespec sleep_time;
	int rc, dim = 0;

	sleep_time.tv_sec = SPL_CHECK_TIME;
	sleep_time.tv_nsec = 0;

	while (1) {
		nanosleep(&sleep_time, NULL);
		
		if ((rc = pthread_mutex_lock(&splchinfo->thinfo.mutex)) != 0) {
			fprintf(stderr, "sp_list_checker_func error - can't acquire lock: %s\n", strerror(rc));
			pthread_exit((void *)-1);
		}

		//controllo se devo terminare
		if (splchinfo->thinfo.go == 0) {
			if ((rc = pthread_mutex_unlock(&splchinfo->thinfo.mutex)) != 0) {
				fprintf(stderr, "sp_list_checker_func error - can't release lock: %s\n", strerror(rc));
				pthread_exit((void *)-1);
			}
			pthread_exit((void *)0);
		}
		
		it = *splchinfo->sp_list;
		dim = 0;
		while (it != NULL) {
			spaddr_tmp = (struct spaddr_node *)it->data;
			tmp_node = it;
			it = it->next;
			if (spaddr_tmp->flag == 0) {
				printf("cancello nodo: %s:%d\n", inet_ntoa(spaddr_tmp->sp_addr.sin_addr), ntohs(spaddr_tmp->sp_addr.sin_port));
				remove_sp(&spaddr_tmp->sp_addr);
			} else {
				dim ++;
				spaddr_tmp->flag = 0;
				printf("Imposto flag: 0 per %s:%d\n", inet_ntoa(spaddr_tmp->sp_addr.sin_addr), ntohs(spaddr_tmp->sp_addr.sin_port));
			}
		}
		printf("trovati %d super peer\n", dim);
		if ((rc = pthread_mutex_unlock(&splchinfo->thinfo.mutex)) != 0) {
			fprintf(stderr, "sp_list_checker_func error - can't release lock: %s\n", strerror(rc));
			pthread_exit((void *)-1);
		}
	}
}

/*
 * Funzione che aggiorna il flag di presenza per il superpeer il cui indirizzo è
 * passato come parametro.
 * Ritorna 0 in caso di successo e -1 in caso di errore.
 */
int update_sp_flag(struct sp_list_checker_info *splchinfo, const struct sockaddr_in *sp_addr) {

	struct spaddr_node *sp_node;
	struct node *tmp_node;
	int rc;

	if ((rc = pthread_mutex_lock(&splchinfo->thinfo.mutex)) != 0) {
		fprintf(stderr, "update_sp_flag error - can't acquire lock: %s\n", strerror(rc));
		return -1;
	}

	tmp_node = get_node_sp(sp_addr);

	if (tmp_node == NULL) {
		printf("Indirizzo non trovato\n");
		if ((rc = pthread_mutex_unlock(&splchinfo->thinfo.mutex)) != 0) {
			fprintf(stderr, "update_sp_flag error - can't release lock: %s\n", strerror(rc));
		}
		return -1;
	}

	sp_node = (struct spaddr_node *)tmp_node->data;

	if (sp_node == NULL) {
		printf("Errore in set_sp_flag: data è NULL\n");
		if ((rc = pthread_mutex_unlock(&splchinfo->thinfo.mutex)) != 0) {
			fprintf(stderr, "update_sp_flag error - can't release lock: %s\n", strerror(rc));
		}
		return -1;	 
	}

	sp_node->flag = 1;

	if ((rc = pthread_mutex_unlock(&splchinfo->thinfo.mutex)) != 0) {
		fprintf(stderr, "update_sp_flag error - can't release lock: %s\n", strerror(rc));
		return -1;
	}

	return 0;
}

