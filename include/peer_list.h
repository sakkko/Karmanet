#ifndef _PEER_LIST_H
#define _PEER_LIST_H


#include "list.h"
#include "inetutil.h"

struct peer_node {
	struct sockaddr_in peer_addr;
	unsigned long peer_rate;
	short flag;
};

struct node *peer_list_head;

void insert_peer(const struct sockaddr_in *peer_addr, long peer_rate);

void sorted_insert_peer(const struct sockaddr_in *peer_addr, long peer_rate);

void remove_peer(const struct sockaddr_in *peer_addr);

void remove_peer_node(struct node *peer_node);

struct node *get_node_peer(const struct sockaddr_in *peer_addr);

void free_list_peer();

struct peer_node *new_peer_node(const struct sockaddr_in *peer_addr, unsigned long rate); 

#endif
