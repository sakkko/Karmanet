#include "pinger.h"

/*
 * Funzione che fa partire il thread pinger.
 * Ritorna 0 in caso di successo e -1 in caso di errore.
 */
int pinger_run(struct pinger_info *pinfo) {
	return thread_run(&pinfo->thinfo, (void *)&pinger_func, (void *)pinfo);
}

/*
 * Funzione che ferma il thread pinger.
 * Ritorna 0 in caso di successo e -1 in caso di errore.
 */
int pinger_stop(struct pinger_info *pinfo) {
	return thread_stop(&pinfo->thinfo);
}

/*
 * Funzione che effettua i ping.
 * In caso di errore il thread termina con valore di ritorno -1, altrimenti, se il thread
 * è interrotto e conclude correttamente il valore di ritorno è 0.
 */
void pinger_func(void *args) {
	struct pinger_info *pinfo = (struct pinger_info *)args;
	struct packet ping_packet;
	struct timespec sleep_time;
	int rc;

	sleep_time.tv_sec = TIME_TO_PING;
	sleep_time.tv_nsec = 0;

	while (1) {
		nanosleep(&sleep_time, NULL);
		if ((rc = pthread_mutex_lock(&pinfo->thinfo.mutex)) != 0) {
			fprintf(stderr, "pinger_func error - can't acquire lock: %s\n", strerror(rc));
			pthread_exit((void *)-1);
		}

		if (pinfo->thinfo.go == 0) {
			if ((rc = pthread_mutex_unlock(&pinfo->thinfo.mutex)) != 0) {
				fprintf(stderr, "pinger_func error - can't release lock: %s\n", strerror(rc));
				pthread_exit((void *)-1);
			}
//			printf("PINGER TERMINATE\n");
			pthread_exit((void *)0);
		}

		if ((rc = pthread_mutex_unlock(&pinfo->thinfo.mutex)) != 0) {
			fprintf(stderr, "pinger_func error - can't release lock: %s\n", strerror(rc));
			pthread_exit((void *)-1);
		}

		new_ping_packet(&ping_packet, get_index());
		if (mutex_send(pinfo->socksd, &pinfo->addr_to_ping, &ping_packet) < 0) {
			fprintf(stderr, "pinger_func error - can't send ping\n");
		}
	}

}

