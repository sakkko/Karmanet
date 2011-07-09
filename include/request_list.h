#ifndef _REQUEST_LIST_H
#define _REQUEST_LIST_H

#include "list.h"
#include "inetutil.h"

struct request_node {
	struct sockaddr_in addr;
	short index;
};


struct node *request_list_head;

void insert_request(const struct sockaddr_in *addr, short index);

void remove_request(const struct sockaddr_in *addr, short index);

void remove_request_node(struct node *request_node);

struct node *get_request_node(const struct sockaddr_in *addr, short index);

void free_list_request();



#endif
