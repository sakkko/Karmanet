#ifndef _FIFO_REQUEST_H
#define _FIFO_REQUEST_H

#include <malloc.h>

#define REQUEST_TTL 3

struct request_node {
	unsigned short id;
	unsigned long ip_sender;
	unsigned short port;
	unsigned short ttl;
	struct request_node* next;
};


struct request_node *request_fifo_tail;

int insert_request(unsigned short id, unsigned long ip, unsigned short port);

void remove_cascade_request(struct request_node *to_remove);

#endif
