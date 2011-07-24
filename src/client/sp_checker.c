#include "sp_checker.h"

/*
 * Funzione che fa partire il thread che controlla il superpeer.
 * Ritorna 0 in caso di successo e -1 in caso di errore.
 */
int sp_checker_run(struct sp_checker_info *chinfo) {
	return thread_run(&chinfo->thinfo, (void *)sp_checker_func, (void *)chinfo);
}

/*
 * Funzione che interrompe il thread che controlla il superpeer.
 * Ritorna 0 in caso di successo e -1 in caso di errore.
 */
int sp_checker_stop(struct sp_checker_info *chinfo) {
	return thread_stop(&chinfo->thinfo);
}

/*
 * Funzione che effettua il controllo del flag di presenza del superpeer .
 * In caso di errore il thread termina con valore di ritorno -1, altrimenti, se il thread
 * è interrotto e conclude correttamente il valore di ritorno è 0.
 */
void sp_checker_func(void *args) {
	struct sp_checker_info *chinfo = (struct sp_checker_info *)args;
	struct timespec sleep_time;
	int rc;

	sleep_time.tv_sec = TIME_CHECK_FLAG;
	sleep_time.tv_nsec = 0;

	printf("AVVIO PROCESSO DI CONTROLLO SP\n");
	while(1){
		nanosleep(&sleep_time, NULL);
		if ((rc = pthread_mutex_lock(&chinfo->thinfo.mutex)) != 0) {
			fprintf(stderr, "sp_checker_func error - can't acquire lock: %s\n", strerror(rc));
			pthread_exit((void *)-1);
		}

		if (chinfo->thinfo.go == 0) {
			if ((rc = pthread_mutex_unlock(&chinfo->thinfo.mutex)) != 0) {
				fprintf(stderr, "sp_checker_func error - can't release lock: %s\n", strerror(rc));
				pthread_exit((void *)-1);
			}
//			printf("SP_CHECKER TERMINATE\n");
			pthread_exit((void *)0);
		}

		if (chinfo->sp_flag == 0) {
			//rimuovere sp
			printf("IL SUPER PEER NON RISPONDE\n");
			write(chinfo->sp_checker_pipe, "RST", 4); //avviso la select di rifare il join al BS
			if ((rc = pthread_mutex_unlock(&chinfo->thinfo.mutex)) != 0) {
				fprintf(stderr, "sp_checker_func error - can't release lock: %s\n", strerror(rc));
				pthread_exit((void *)-1);
			}
//			printf("SP_CHECKER RESET\n");
			pthread_exit((void *)0);
		} else {
			chinfo->sp_flag = 0;
		}

		if ((rc = pthread_mutex_unlock(&chinfo->thinfo.mutex)) != 0) {
			fprintf(stderr, "sp_checker_func error - can't release lock: %s\n", strerror(rc));
			pthread_exit((void *)-1);
		}
	}

}

/*
 * Funzione che aggiorna il flag di presenza del superpeer.
 * Ritorna 0 in caso di successo e -1 in caso di errore.
 */
int update_sp_flag(struct sp_checker_info *chinfo){
	int rc;

	if ((rc = pthread_mutex_lock(&chinfo->thinfo.mutex)) != 0) {
		fprintf(stderr, "update_sp_flag error - can't acquire lock: %s\n", strerror(rc));
		return -1;
	}

	chinfo->sp_flag = 1;

	if ((rc = pthread_mutex_unlock(&chinfo->thinfo.mutex)) != 0) {
		fprintf(stderr, "update_sp error_flag - can't release lock: %s\n", strerror(rc));
		return -1;
	}

	return 0;
}
