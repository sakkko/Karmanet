	#include "fifo_request.h"

void insert_request(unsigned short id, unsigned long ip){
	
	struct request_node* iterator = request_fifo_tail;
	
	if(iterator != NULL){
		while(iterator->next != NULL){
		
			if(iterator->ip_sender == ip && iterator->id == id)
				return;
		
		}
		
	}
	
	struct request_node *ret;
	
	ret = (struct request_node *)malloc(sizeof(struct request_node));
	ret->next = request_fifo_tail;
	ret->id = id;
	ret->ip_sender = ip;
//	ret->ttl = DEFAULT_TTL;
	request_fifo_tail = ret;
		
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

