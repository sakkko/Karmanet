#ifndef _THREAD_UTIL_H
#define _THREAD_UTIL_H

#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <unistd.h>
#include <time.h>

#include "command.h"

struct th_info {
	pthread_t thread;
	pthread_mutex_t mutex;
	int go;
};

int thread_run(struct th_info *thinfo, void *(*start_routine)(void *), void *args);

int thread_stop(struct th_info *thinfo);

int write_err(int th_pipe, pthread_mutex_t *pipe_mutex, const char *msg);

int write_on_pipe(int th_pipe, pthread_mutex_t *pipe_mutex, const char *str);

#endif
