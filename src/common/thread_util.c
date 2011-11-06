#include "thread_util.h"

int write_err(int th_pipe, pthread_mutex_t *pipe_mutex, const char *msg) {
	int rc;
	char str[32];

	sprintf(str, "ERR %s\n", msg);
	if ((rc = pthread_mutex_lock(pipe_mutex)) != 0) {
		fprintf(stderr, "write_err error - can't acquire lock on pipe: %s\n", strerror(rc));
		return -1;
	}
	if (write(th_pipe, str, strlen(str)) < 0) {
		if ((rc = pthread_mutex_unlock(pipe_mutex)) != 0) {
			fprintf(stderr, "write_err error - can't release lock on pipe: %s\n", strerror(rc));
			return -1;
		}
		perror("write_err error - can't write on pipe");
		return -1;
	}
	if ((rc = pthread_mutex_unlock(pipe_mutex)) != 0) {
		fprintf(stderr, "write_err error - can't release lock on pipe: %s\n", strerror(rc));
		return -1;
	}

	return 0;
}

int write_on_pipe(int th_pipe, pthread_mutex_t *pipe_mutex, const char *str) {
	int rc;

	if ((rc = pthread_mutex_lock(pipe_mutex)) != 0) {
		fprintf(stderr, "write_err error - can't acquire lock on pipe: %s\n", strerror(rc));
		return -1;
	}
	if (write(th_pipe, str, strlen(str)) < 0) {
		if ((rc = pthread_mutex_unlock(pipe_mutex)) != 0) {
			fprintf(stderr, "write_err error - can't release lock on pipe: %s\n", strerror(rc));
			return -1;
		}
		perror("write_err error - can't write on pipe");
		return -1;
	}
	if ((rc = pthread_mutex_unlock(pipe_mutex)) != 0) {
		fprintf(stderr, "write_err error - can't release lock on pipe: %s\n", strerror(rc));
		return -1;
	}

	return 0;
}

/*
 * Funzione che lancia un thread, che eseguirà la funzione start_routine con parametri args.
 * Ritorna 0 in caso di successo e -1 in caso di errore.
 */
int thread_run(struct th_info *thinfo, void *(*start_routine)(void *), void *args) {
	int rc;
	pthread_attr_t attr;

	if ((rc = pthread_attr_init(&attr)) != 0) {
		fprintf(stderr, "thread_run error - can't initialize thread attribute: %s\n", strerror(rc));
		return -1;
	}

	if ((rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE)) != 0) {
		fprintf(stderr, "thread_run error - can't set thread attribute: %s\n", strerror(rc));
		return -1;
	}

	if ((rc = pthread_mutex_init(&thinfo->mutex, NULL)) != 0) {
		fprintf(stderr, "thread_run error - can't initialize lock: %s\n", strerror(rc));
		return -1;
	}

	thinfo->go = 1;

	if ((rc = pthread_create(&thinfo->thread, &attr, start_routine, args)) != 0) {
		fprintf(stderr, "thread_run error - can't create thread: %s\n", strerror(rc));
		return -1;
	}

	return 0;

}

/*
 * Funzione che ferma un thread. Questa funzione blocca finchè il thread specificato
 * non è effettivamente interrotto.
 * Ritorna 0 in caso di successo e -1 in caso di errore.
 */
int thread_stop(struct th_info *thinfo) {
	int rc;

	if ((rc = pthread_mutex_lock(&thinfo->mutex)) != 0) {
		fprintf(stderr, "thread_stop error - can't acquire lock: %s\n", strerror(rc));
		return -1;
	}
	thinfo->go = 0;

	if ((rc = pthread_mutex_unlock(&thinfo->mutex)) != 0) {
		fprintf(stderr, "thread_stop error - can't release lock: %s\n", strerror(rc));
		return -1;
	}

	if ((rc = pthread_kill(thinfo->thread, SIGALRM)) != 0) {
		fprintf(stderr, "thread_stop error - can't send signal to thread\n");
		return -1;
	}

/*
	if ((rc = pthread_join(thinfo->thread, (void *)&ret_value)) != 0) {
		fprintf(stderr, "pinger_stop error - can't join thread: %s\n", strerror(rc));
		return -1;
	}
*/
	return 0;

}

