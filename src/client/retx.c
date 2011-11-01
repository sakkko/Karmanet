#include "retx.h"

pthread_mutex_t retx_mutex = PTHREAD_MUTEX_INITIALIZER;

void retx_func(void *args) {
	struct retx_info *rtxinfo = (struct retx_info *)args;
	char cmdstr[CMD_STR_LEN + 1];
	struct node *iterator;
	struct packet_node *pck_node;
	struct timespec sleep_time;
	int rc;

	sleep_time.tv_sec = TIME_TO_SLEEP;
	sleep_time.tv_nsec = 0;

	while (1) {
		nanosleep(&sleep_time, NULL);
		if ((rc = pthread_mutex_lock(&retx_mutex)) != 0) {
			fprintf(stderr, "retx_func error - can't acquire lock: %s\n", strerror(rc));
			pthread_exit((void *)-1);
		}

		iterator = packet_list_head;

		while (iterator != NULL) {
			pck_node = (struct packet_node *)iterator->data;

			if (pck_node->countdown > 0) {
				pck_node->countdown --;
			} else if (pck_node->countdown == 0) {
				strncpy(cmdstr, pck_node->pck.cmd, CMD_STR_LEN);
				cmdstr[CMD_STR_LEN] = 0;
				printf("Pacchetto: INDEX=%u CMD=%s NRETX=%d COUNTDOWN %d\n", pck_node->pck.index, cmdstr, pck_node->nretx, pck_node->countdown);
				if (pck_node->nretx == MAX_RETX || pck_node->errors == MAX_RETX) {
					if (write_err(rtxinfo->retx_wr_pipe, rtxinfo->pipe_mutex, cmdstr) < 0) {
						fprintf(stderr, "retx_func error - write_err failed: %s\n", strerror(rc));
						pthread_exit((void *)-1);
					}
					pck_node->countdown = -1;
					iterator = iterator->next;
					continue;
				}

				if (send_packet(pck_node->socksd, &pck_node->addrto, &pck_node->pck) < 0) {
					fprintf(stderr, "retx_func error - send_packet failed\n");
					pck_node->countdown = pck_node->nretx;
					pck_node->errors ++;
					iterator = iterator->next;
					continue;
				}

				pck_node->nretx ++;
				pck_node->countdown = pck_node->nretx;
			}

			iterator = iterator->next;
		}

		if ((rc = pthread_mutex_unlock(&retx_mutex)) != 0) {
			fprintf(stderr, "retx_func error - can't release lock: %s\n", strerror(rc));
			pthread_exit((void *)-1);
		}

	}
}

int retx_send(int socksd, const struct sockaddr_in *addr, const struct packet *pck) {
	int rc;

	if ((rc = pthread_mutex_lock(&retx_mutex)) != 0) {
		fprintf(stderr, "retx_send error - can't acquire lock: %s\n", strerror(rc));
		return -1;
	}

	if (send_packet(socksd, addr, pck) < 0) {
		fprintf(stderr, "retx_send error - send_packet failed\n");
		return -1;
	}

	insert_packet(socksd, pck, addr, 1, 1, 0);

	if ((rc = pthread_mutex_unlock(&retx_mutex)) != 0) {
		fprintf(stderr, "retx_send error - can't release lock: %s\n", strerror(rc));
		return -1;
	}

	return 0;
}

int retx_stop(int pck_index, char *pck_cmd) {
	struct node *tmp_node;
	int rc;

	tmp_node = get_packet_node(pck_index);

	if (tmp_node == NULL) {
		return 1;
	}

	if (pck_cmd != NULL) {
		strncpy(pck_cmd, ((struct packet_node *)tmp_node->data)->pck.cmd, CMD_STR_LEN);
	}

	if ((rc = pthread_mutex_lock(&retx_mutex)) != 0) {
		fprintf(stderr, "retx_stop error - can't acquire lock: %s\n", strerror(rc));
		return -1;
	}

	remove_packet_node(tmp_node);

	if ((rc = pthread_mutex_unlock(&retx_mutex)) != 0) {
		fprintf(stderr, "retx_stop error - can't release lock: %s\n", strerror(rc));
		return -1;
	}

	return 0;
}

int retx_run(struct retx_info *rtxinfo) {
	int rc;

	if ((rc = pthread_create(&rtxinfo->thread, NULL, (void *)retx_func, (void *)rtxinfo)) != 0) {
		fprintf(stderr, "thread_run error - can't create thread: %s\n", strerror(rc));
		return -1;
	}

	return 0;
}

int mutex_send(int socksd, const struct sockaddr_in *addr, const struct packet *pck) {
	int rc, ret;

	ret = 0;

	if ((rc = pthread_mutex_lock(&retx_mutex)) != 0) {
		fprintf(stderr, "mutx_send error - can't acquire lock: %s\n", strerror(rc));
		return -1;
	}

	if (send_packet(socksd, addr, pck) < 0) {
		perror("mutex_send error - send_packet failed");
		ret = -1;
	}

	if ((rc = pthread_mutex_unlock(&retx_mutex)) != 0) {
		fprintf(stderr, "mutex_send error - can't release lock: %s\n", strerror(rc));
		return -1;
	}

	return ret;
}

