#ifndef _PEER_LIST_CHECKER_H
#define _PEER_LIST_CHECKER_H

#include <netinet/in.h>

#include "list.h"
#include "peer_list.h"
#include "thread_util.h"
#include "hashtable.h"
#include "fifo_request.h"
#include "state.h"
#include "packet_util.h"

#define PL_CHECK_TIME 6

struct peer_list_ch_info {
	struct th_info thinfo;
	struct node **peer_list;
	pthread_mutex_t request_mutex;
};

int peer_list_checker_run(struct peer_list_ch_info *plchinfo);

int peer_list_checker_stop(struct peer_list_ch_info *plchinfo);

void peer_list_checker_func(void *args);

int update_peer_flag(struct peer_list_ch_info *plchinfo, const struct sockaddr_in *peer_addr, const struct packet *recv_pck);

#endif
