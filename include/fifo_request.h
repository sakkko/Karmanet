#ifndef _FIFO_REQUEST_H
#define _FIFO_REQUEST_H

#include "list.h"

#define REQUEST_TTL 3

struct request_node {
	unsigned short id;
	unsigned long ip_sender;
	unsigned short port;
	unsigned short ttl;
};


struct node *request_list_head;

/*
* Funzione che inserisce un indirizzo in testa alla lista.
*/
int insert_request(unsigned short id, unsigned long ip_sender, unsigned short port);

struct request_node *new_request_node(unsigned short id, unsigned long ip_sender, unsigned short port);

void remove_request(unsigned short id, unsigned long ip_sender, unsigned short port);

struct node *get_request_node(unsigned short id, unsigned long ip_sender, unsigned short port);

void remove_request_node(struct node *request_node);

void free_list_request();

#endif
