#ifndef _NEAR_LIST_H
#define _NEAR_LIST_H

#include <malloc.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "inetutil.h"
#include "thread_util.h"

struct near_node {
	int socksd;
	struct sockaddr_in addr;
	char data[36];
	pthread_mutex_t mutex;
	short near_number;

	struct near_node *next;
	struct near_node *prev;
};

struct near_node *near_list_head;

pthread_mutex_t NEAR_LIST_LOCK;


int insert_near(int socksd, const struct sockaddr_in *addr);

int update_near(int socksd, const char *data, unsigned int data_len);

struct near_node *get_near_node(int socksd);

struct near_node *create_new_near_node(int socksd, const struct sockaddr_in *addr);
	
int delete_near(int socksd);

struct near_node *remove_near_node(struct near_node *head, struct near_node *toremove);

#endif
