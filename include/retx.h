#ifndef _RETX_H
#define _RETX_H


#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>

#include "inetutil.h"
#include "packet_util.h"

#define NTHREADS 3
#define DEFAULT_TO 5
#define MAX_TO 15

struct send_info {
	int socksd;
	struct packet pck;
	struct sockaddr_in addrto;
};

struct retx_info {
	pthread_t thread;
	int index;
	struct send_info snd_info;
	sem_t sem;
	pthread_mutex_t *pipe_mutex;
	int retx_wr_pipe;
};


struct retx_info retx_threads[NTHREADS];

sem_t rtx_sem, pck_sem;

struct send_info packet_to_send;

void retx_func(void *args);

int retx_send(int socksd, const struct sockaddr_in *addr, const struct packet *pck);

int retx_stop(int pck_index);

int retx_init(int wr_pipe, pthread_mutex_t *pipe_mutex);

int retx_recvfrom(int socksd, struct sockaddr_in *addr, struct packet *pck, int *addr_len);

int retx_recv(int socksd, struct packet *pck);

int mutex_send(int socksd, const struct sockaddr_in *addr, const struct packet *pck);

#endif
