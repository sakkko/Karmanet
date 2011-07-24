#ifndef _PINGER_H
#define _PINGER_H

#include <pthread.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/time.h>

#include "packet_util.h"
#include "retx.h"
#include "thread_util.h"

#define TIME_TO_PING  2

struct pinger_info {
	struct th_info thinfo;
	int socksd;
	struct sockaddr_in *addr_to_ping;
};

int pinger_run(struct pinger_info *pinfo);

int pinger_stop(struct pinger_info *pinfo);

void pinger_func(void *args);


#endif
