#include "retx.h"

pthread_mutex_t retx_mutex = PTHREAD_MUTEX_INITIALIZER;

int get_pck_tosend(struct retx_info *rtxinfo) {
	if (sem_wait(&rtx_sem) < 0) {
		perror("get_pck_tosend error - sem_wait failed");
		return -1;
	}
	rtxinfo->snd_info.socksd = packet_to_send.socksd;
	addrcpy(&rtxinfo->snd_info.addrto, &packet_to_send.addrto);
	pckcpy(&rtxinfo->snd_info.pck, &packet_to_send.pck);
	
	if (sem_post(&pck_sem) < 0) {
		perror("get_pck_tosend error - sem_post failed");
		return -1;
	}

	return 0;
}

int write_err(const struct retx_info *rtxinfo, char *msg) {
	int rc;
	char str[10];

	snprintf(str, 10, "ERR %s\n", msg);
	if ((rc = pthread_mutex_lock(rtxinfo->pipe_mutex)) != 0) {
		fprintf(stderr, "write_err error - can't acquire lock on pipe: %s\n", strerror(rc));
		return -1;
	}
	if (write(rtxinfo->retx_wr_pipe, str, strlen(str)) < 0) {
		if ((rc = pthread_mutex_unlock(rtxinfo->pipe_mutex)) != 0) {
			fprintf(stderr, "write_err error - can't release lock on pipe: %s\n", strerror(rc));
			return -1;
		}
		perror("write_err error - can't write on pipe");
		return -1;
	}
	if ((rc = pthread_mutex_unlock(rtxinfo->pipe_mutex)) != 0) {
		fprintf(stderr, "write_err error - can't release lock on pipe: %s\n", strerror(rc));
		return -1;
	}

	return 0;
}

void retx_func(void *args) {
	struct retx_info *rtxinfo = (struct retx_info *)args;
	struct timeval now;
	struct timespec timeout;
	int to = 0;
	int rc, errors = 0;
	int indx = rtxinfo->index;	
	char cmdstr[CMD_STR_LEN + 1];

	while (1) {
		if (get_pck_tosend(rtxinfo) < 0) {
			fprintf(stderr, "retx_func error - get_pck_tosend failed\n");
			continue;
		}
		to = 0;
		errors = 0;
		while (1) {
			strncpy(cmdstr, rtxinfo->snd_info.pck.cmd, CMD_STR_LEN);
			cmdstr[CMD_STR_LEN] = 0;
			printf("Thread %d: cmd: %s, index: %d timeout: %d sec\n", indx, cmdstr, rtxinfo->snd_info.pck.index, DEFAULT_TO);
			if ((rc = pthread_mutex_lock(&retx_mutex)) != 0) {
				fprintf(stderr, "retx_func error - can't acquire lock: %s\n", strerror(rc));
				pthread_exit((void *)-1);
			}
			if (send_packet(rtxinfo->snd_info.socksd, &rtxinfo->snd_info.addrto, &rtxinfo->snd_info.pck) < 0) {
				if ((rc = pthread_mutex_unlock(&retx_mutex)) != 0) {
					fprintf(stderr, "retx_func error - can't release lock: %s\n", strerror(rc));
					pthread_exit((void *)-1);
				}
				perror("errore in sendto");
				errors ++;
				if (errors < 5) {
					continue;
				}

				if (write_err(rtxinfo, cmdstr) < 0) {
					fprintf(stderr, "retx_func error - write_err failed: %s\n", strerror(rc));
					pthread_exit((void *)-1);
				}
				break;
			}
			if ((rc = pthread_mutex_unlock(&retx_mutex)) != 0) {
				fprintf(stderr, "retx_func error - can't release lock: %s\n", strerror(rc));
				pthread_exit((void *)-1);
			}
			gettimeofday(&now, NULL);
			timeout.tv_sec = now.tv_sec + DEFAULT_TO;
			timeout.tv_nsec = now.tv_usec * 1000;
			if ((rc = sem_timedwait(&rtxinfo->sem, &timeout)) != 0) {
				if (errno == ETIMEDOUT) {
					to ++;
					if (to < 5) {
						printf("RITRASMETTO\n");
						continue;					
					} else {
						if (write_err(rtxinfo, cmdstr) < 0) {
							fprintf(stderr, "retx_func error - write_err failed: %s\n", strerror(rc));
							pthread_exit((void *)-1);
						}
						break;
					}
				} else {
					perror("errore in sem_timedwait");
				}
			} else {
				if ((rc = pthread_mutex_lock(&retx_mutex)) != 0) {
					fprintf(stderr, "retx_func error - can't acquire lock: %s\n", strerror(rc));
					pthread_exit((void *)-1);
				}
				rtxinfo->snd_info.pck.index = -1;
				if ((rc = pthread_mutex_unlock(&retx_mutex)) != 0) {
					fprintf(stderr, "retx_func error - can't release lock: %s\n", strerror(rc));
					pthread_exit((void *)-1);
				}
				break;
			}
		}
	}

}

int retx_send(int socksd, const struct sockaddr_in *addr, const struct packet *pck) {
//	printf("============ENTRO IN RETX_SEND=========\n");
	
	if (sem_wait(&pck_sem) < 0) {
		perror("retx_send error - sem_wait failed");
		return -1;
	}
	packet_to_send.socksd = socksd;
	addrcpy(&packet_to_send.addrto, addr);
	pckcpy(&packet_to_send.pck, pck);
	if (sem_post(&rtx_sem) < 0) {
		perror("retx_send error - sem_post failed");
		return -1;
	}

//	printf("============ESCO DA RETX_SEND=========\n");
	return 0;
	
}

int try_retx_send(int socksd, const struct sockaddr_in *addr, const struct packet *pck) {
//	printf("============ENTRO IN RETX_SEND=========\n");
	
	if (sem_trywait(&pck_sem) < 0) {
		if (errno == EAGAIN) {
			return -2;
		}
		perror("retx_send error - sem_wait failed");
		return -1;
	}
	packet_to_send.socksd = socksd;
	addrcpy(&packet_to_send.addrto, addr);
	pckcpy(&packet_to_send.pck, pck);
	if (sem_post(&rtx_sem) < 0) {
		perror("retx_send error - sem_post failed");
		return -1;
	}

//	printf("============ESCO DA RETX_SEND=========\n");
	return 0;
	
}

int retx_stop(int pck_index) {
//	printf("\n============ENTRO IN RETX_STOP: pck_index:%d =========\n", pck_index);
	int i;
	
	for (i = 0; i < NTHREADS; i ++) {
		if (retx_threads[i].snd_info.pck.index == pck_index) {
			break;
		} 
	}
	
	if (i == NTHREADS) {
		printf("retx_stop error - no packet found with index %d\n", pck_index);
		return 1;
	}
	
	if (sem_post(&retx_threads[i].sem) < 0) {
		perror("retx_stop error - sem_post failed");
		return -1;
	}
//	printf("============ESCO DA RETX_STOP=========\n");
	return 0;
}

int retx_init(int wr_pipe, pthread_mutex_t *pipe_mutex) {
	int i, rc;
	if (sem_init(&rtx_sem, 0, 0) < 0) {
		perror("retx_init error sem_init failed");
		return -1;
	}
	if (sem_init(&pck_sem, 0, 1) < 0) {
		perror("retx_init error sem_init failed");
		return -1;
	}
	for (i = 0; i < NTHREADS; i ++) {
		if (sem_init(&retx_threads[i].sem, 0, 0) < 0) {
			perror("retx_init error sem_init failed");
			return -1;
		}
		retx_threads[i].index = i;
		retx_threads[i].retx_wr_pipe = wr_pipe;
		retx_threads[i].pipe_mutex = pipe_mutex;
		if ((rc = pthread_create(&retx_threads[i].thread, NULL, (void *)&retx_func, (void *)&retx_threads[i])) != 0) {
			fprintf(stderr,  "retx_init error - pthread_create failed: %s\n", strerror(rc));
		 	return -1;
		} 
	}
	return 0;
}

int mutex_send(int socksd, const struct sockaddr_in *addr, const struct packet *pck) {
	pthread_mutex_lock(&retx_mutex);
	if (send_packet(socksd, addr, pck) < 0) {
		pthread_mutex_unlock(&retx_mutex);
		perror("mutex_send error");
		return -1;
	}
	pthread_mutex_unlock(&retx_mutex);
	return 0;
}

