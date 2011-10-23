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
		
		if (is_sp){
			if (tcp_ping() < 0) {
				fprintf(stderr, "pinger_func error - tcp_ping failed\n");
				pthread_exit((void *)-1);
			}
		}	
	}

}

int tcp_ping(struct pinger_info *pinfo) {
	struct near_node *iterator;
	struct packet ping_packet;
	int rc;

	new_ping_packet(&ping_packet, 0);

//	printf("tcp_ping ottengo lock\n");
	if ((rc = pthread_mutex_lock(&NEAR_LIST_LOCK)) != 0) {
		fprintf(stderr, "tcp_ping error - can't acquire lock: %s\n", strerror(rc));
		return -1;
	}
//	printf("tcp_ping lock ottenuto\n");

	add_near_to_packet(&ping_packet, near_str, MAX_TCP_SOCKET * 6);
	ping_packet.data_len = 6 * nsock;

	iterator = near_list_head;
	while (iterator != NULL) {
		ping_packet.index = get_index();	
		if ((rc = pthread_mutex_lock(&iterator->mutex)) != 0) {
			fprintf(stderr, "tcp_ping error - can't acquire tcp lock: %s\n", strerror(rc));
			if ((rc = pthread_mutex_unlock(&NEAR_LIST_LOCK)) != 0) {
				fprintf(stderr, "tcp_ping error - can't release lock: %s\n", strerror(rc));
			}
			return -1;
		}

		if(send_packet_tcp(iterator->socksd, &ping_packet) < 0){
			fprintf(stderr, "tcp_ping error - send_packet_tcp failed \n");
			if ((rc = pthread_mutex_unlock(&iterator->mutex)) != 0) {
				fprintf(stderr, "tcp_ping error - can't release lock: %s\n", strerror(rc));
			}
			if ((rc = pthread_mutex_unlock(&NEAR_LIST_LOCK)) != 0) {
				fprintf(stderr, "tcp_ping error - can't release lock: %s\n", strerror(rc));
			}
			return -1;
		}

		if ((rc = pthread_mutex_unlock(&iterator->mutex)) != 0) {
			if ((rc = pthread_mutex_unlock(&NEAR_LIST_LOCK)) != 0) {
				fprintf(stderr, "tcp_ping error - can't release lock: %s\n", strerror(rc));
			}

			fprintf(stderr, "tcp_ping error - can't release lock: %s\n", strerror(rc));
			return -1;
		}

		iterator = iterator->next;
	}

	if ((rc = pthread_mutex_unlock(&NEAR_LIST_LOCK)) != 0) {
		fprintf(stderr, "tcp_ping error - can't release lock: %s\n", strerror(rc));
		return -1;
	}
//	printf("tcp_ping rilascio lock\n");

	return 0;
}

