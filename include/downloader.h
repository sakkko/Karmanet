#ifndef _DOWNLOADER_H
#define _DOWNLOADER_H

#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>

#include "inetutil.h"
#include "packet_util.h"
#include "transfer_list.h"
#include "thread_util.h"

#define TIME_TO_SLEEP 1

struct downloader_info {
	pthread_t thread;
	pthread_mutex_t *pipe_mutex;
	int th_pipe;
};

struct downloader_info *dw_pool;

int downloader_init(int nthread, int th_pipe, pthread_mutex_t *pipe_mutex);

int downloader_run(struct download_info *dwinfo);

void download_func(void *args);

int add_download(const struct sockaddr_in *addr, const unsigned char *md5);

int write_err(const struct retx_info *rtxinfo, char *msg);


#endif
