#ifndef _LIST_H
#define _LIST_H

#include <malloc.h>

struct node {
	void *data;
	struct node *next;
  	struct node *prev;
};

struct node *insert_node(struct node *list, void *data);

struct node *sorted_insert_node(struct node *list, struct node *prev_node, void *data); 

struct node *remove_node(struct node *list, struct node *toremove);

void free_list(struct node *list); 

#endif
