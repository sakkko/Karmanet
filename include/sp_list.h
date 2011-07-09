#ifndef _SP_LIST_H
#define _SP_LIST_H

#include "list.h"
#include "inetutil.h"

#define ADDR_TOSEND 4


struct spaddr_node {
	struct sockaddr_in sp_addr;
	short flag;
};

struct node *sp_list_head;

struct node *it_addr;

int sp_count;


void insert_sp(const struct sockaddr_in *sp_addr);

void remove_sp(const struct sockaddr_in *sp_addr);

void remove_sp_node(struct node *sp_node);

struct node *get_node_sp(const struct sockaddr_in *sp_addr);

struct sockaddr_in *get_addr();

void free_list_sp();

#endif
