#ifndef _RETX_H
#define _RETX_H


#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>

#include "inetutil.h"
#include "packet_util.h"
#include "packet_list.h"
#include "thread_util.h"

#define TIME_TO_SLEEP 1
#define DEFAULT_TO 5
#define MAX_RETX 6

struct retx_info {
	pthread_t thread;
	pthread_mutex_t *pipe_mutex;
	int retx_wr_pipe;
};


void retx_func(void *args);

int retx_send(int socksd, const struct sockaddr_in *addr, const struct packet *pck);

int retx_stop(int pck_index, char *pck_cmd);

int retx_run(struct retx_info *rtxinfo);

int mutex_send(int socksd, const struct sockaddr_in *addr, const struct packet *pck);

#endif
