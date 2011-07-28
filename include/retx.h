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


struct retx_info {
	int index;
	int socksd;
	struct sockaddr_in addrto;
	struct packet pck;
	sem_t sem;
};

struct thread_info {
	pthread_t threads[NTHREADS];
	struct retx_info th_retx_info[NTHREADS];
};

sem_t rtx_sem, pck_sem;

struct thread_info thinfo;

struct retx_info packet_to_send;


void retx_func(void *args);

int retx_send(int socksd, const struct sockaddr_in *addr, const struct packet *pck);

int retx_stop(int pck_index);

int retx_init();

int retx_recvfrom(int socksd, struct sockaddr_in *addr, struct packet *pck, int *addr_len);

int retx_recv(int socksd, struct packet *pck);

int mutex_send(int socksd, const struct sockaddr_in *addr, const struct packet *pck);

#endif
