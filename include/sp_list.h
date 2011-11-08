#ifndef _SP_LIST_H
#define _SP_LIST_H

#include "list.h"
#include "inetutil.h"

struct spaddr_node {
	struct sockaddr_in sp_addr;
	unsigned char free_spots;
	short flag;
};

struct node *sp_list_head;

struct node *it_addr;

int sp_count;

int addr_to_send;

void insert_sp(const struct sockaddr_in *sp_addr, unsigned char free_spot);

void remove_sp(const struct sockaddr_in *sp_addr);

void remove_sp_node(struct node *sp_node);

struct node *get_node_sp(const struct sockaddr_in *sp_addr);

struct sockaddr_in *get_addr();

struct node *get_addr_sp(int *dim);

void free_list_sp();

#endif
