#ifndef _PACKET_LIST_H
#define _PACKET_LIST_H

#include "packet_util.h"
#include "list.h"

struct packet_node {
	int socksd;
	struct packet pck;
	struct sockaddr_in addrto;

	short nretx;
	short countdown;
	short errors;
};

struct node *packet_list_head;

int packet_count;

void insert_packet(int socksd, const struct packet *pck, const struct sockaddr_in *addr, short nretx, short countdown, short errors);

struct packet_node *new_packet_node(int socksd, const struct packet *pck, const struct sockaddr_in *addr, short nretx, short countdown, short errors);

void remove_packet(unsigned short pck_index);

struct node *get_packet_node(unsigned short pck_index);

void remove_packet_node(struct node *packet_node);

void free_packet_list();

#endif
