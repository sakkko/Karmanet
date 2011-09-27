#include "request_fifo_checker.h"

/*
 * Funzione che lancia il thread che controlla la fifo delle request.
 * Ritorna 0 in caso di successo e -1 in caso di errore.
 */
 int request_fifo_checker_run(struct request_fifo_ch_info *rfchinfo ){
	return thread_run(&rfchinfo->thinfo, (void *)&request_fifo_checker_func, (void *)rfchinfo );
}

/*
 * Funzione che ferma il thread che controlla la fifo delle request.
 * Ritorna 0 in caso di successo e -1 in caso di errore.
 */
 
int request_fifo_checker_stop(struct request_fifo_ch_info *rfchinfo ) {
	return thread_stop(&rfchinfo ->thinfo);
}

/*
 * Funzione che esegue il controllo della fifo delle request.
 * In caso di errore il thread termina con valore di ritorno -1, altrimenti, se il thread
 * è interrotto e conclude correttamente il valore di ritorno è 0.
 */

void request_fifo_checker_func(void *args) {
	struct request_fifo_ch_info *rfchinfo  = (struct request_fifo_ch_info *)args;
	struct timespec sleep_time;
	struct request_node *tmp_node;
	struct request_node *to_remove;
	int rc;

	printf("AVVIO PROCESSO DI CONTROLLO FIFO REQUEST\n");

	sleep_time.tv_sec = RF_CHECK_TIME;
	sleep_time.tv_nsec = 0;

	while (1) {
		nanosleep(&sleep_time, NULL);

		if ((rc = pthread_mutex_lock(&rfchinfo->thinfo.mutex)) != 0) {
			fprintf(stderr, "request_fifo_checker_func error - can't acquire lock: %s\n", strerror(rc));
			pthread_exit((void *)-1);
		}

		if (rfchinfo->thinfo.go == 0) {
			if ((rc = pthread_mutex_unlock(&rfchinfo->thinfo.mutex)) != 0) {
				fprintf(stderr, "request_fifo_checker_func error - can't release lock: %s\n", strerror(rc));
				pthread_exit((void *)-1);
			}
			pthread_exit((void *)0);
		}
		
		tmp_node = request_fifo_tail;		
		
		if(tmp_node != NULL){
			if (tmp_node->ttl == 0) {
					remove_cascade_request(tmp_node);
					tmp_node->next = NULL;
			} else {
					tmp_node->ttl --;	
			}
			while (tmp_node->next != NULL) {
			
				to_remove = tmp_node->next;		
				if (to_remove->ttl == 0) {
					remove_cascade_request(to_remove);
					tmp_node->next = NULL;
				} else {
					to_remove->ttl --;	
				}

			}
		}

		if ((rc = pthread_mutex_unlock(&rfchinfo ->thinfo.mutex)) != 0) {
			fprintf(stderr, "request_fifo_checker_func error - can't release lock: %s\n", strerror(rc));
			pthread_exit((void *)-1);
		}
	}

}

