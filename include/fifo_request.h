#ifndef _FIFO_REQUEST_H
#define _FIFO_REQUEST_H

#include <malloc.h>

struct request_node {
	
	unsigned short id;
	unsigned long ip_sender;
	unsigned short ttl;
	struct request_node* next;
	};


struct node *request_fifo_tail;

#define DEFAULT_TTL 5;

void insert_request(unsigned short id, unsigned long ip);

void remove_cascade_request(const struct request_node *to_remove);

#endif
