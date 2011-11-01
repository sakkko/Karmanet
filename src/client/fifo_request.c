#include "fifo_request.h"

int insert_request(unsigned short id, unsigned long ip, unsigned short port){
	struct request_node *ret;
	struct request_node* iterator = request_fifo_tail;
	
	while (iterator != NULL) {
		if (iterator->ip_sender == ip && iterator->id == id && iterator->port == port) {			
			return 1;
		}
		if (iterator->next == NULL) {
			break;
		}
		iterator = iterator->next;
	}

	
	ret = (struct request_node *)malloc(sizeof(struct request_node));
	ret->next = request_fifo_tail;
	ret->id = id;
	ret->port = port;
	ret->ip_sender = ip;
	ret->ttl = REQUEST_TTL;
	request_fifo_tail = ret;

	return 0;
}

/*
la funzione elimina in cascata tutti i nodi a partire dal nodo di ingresso 
*/	
void remove_cascade_request(struct request_node *to_remove){

	if(to_remove == NULL)
		return;

	struct request_node *iterator = to_remove->next;

	free(to_remove);
	to_remove = NULL;

	return(remove_cascade_request(iterator));		
}

