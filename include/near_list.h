#ifndef _NEAR_LIST_H
#define _NEAR_LIST_H



#include <malloc.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//#include "inetutil.h"

struct near_node{
	struct sockaddr_in near_addr;
	char data[36];
	short near_number;
	struct near_node *next;
	struct near_node *prev;
};



struct near_node *near_list_head;



void insert_near(const struct sockaddr_in *addr, const char *data, unsigned int data_len);
char * get_near_data(const struct sockaddr_in *addr);
struct near_node * create_new_near_node(const struct sockaddr_in *addr, const char *data, int data_len);
void delete_near_addr(const struct sockaddr_in *addr);
struct near_node *remove_near_node(struct near_node *head, struct near_node *toremove);
#endif
