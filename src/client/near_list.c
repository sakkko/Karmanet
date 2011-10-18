//near list
#include "near_list.h"


void insert_near(const struct sockaddr_in *addr, const char *data, unsigned int data_len){
	
	
	if(near_list_head == NULL){
	//creo testa	
		near_list_head = create_new_near_node(addr,data,data_len);
		near_list_head->next = NULL;
		near_list_head->prev = NULL;
		return;
	}
	struct near_node * iterator = near_list_head;
	while(iterator != NULL){
		
		if(!memcmp(&(iterator->near_addr), addr, sizeof(struct sockaddr_in) )){
			memcpy(iterator->data,data,data_len);
			return;		
		}
		
		iterator = iterator->next;
	}
	//nodo non trovato inserisco in testa
	struct near_node * to_create = create_new_near_node(addr,data,data_len);
	to_create->next = near_list_head->next;
	to_create->prev = near_list_head;
	
	if(near_list_head->next != NULL){
		near_list_head->next->prev = to_create;
		
	}
	near_list_head->next = to_create;
	
	return;
}

char * get_near_data(const struct sockaddr_in *addr){
	
	
	
	struct near_node * iterator = near_list_head;
	while(iterator != NULL){
		
		if(!memcmp(&(iterator->near_addr), addr, sizeof(struct sockaddr_in) )){

			return iterator->data;		
		}
		
		iterator = iterator->next;
	}
	return NULL;
	
}

struct near_node * create_new_near_node(const struct sockaddr_in *addr, const char *data, int data_len){
	
	struct near_node * ret = (struct near_node *)malloc(sizeof(struct near_node));
	ret->near_addr = *addr;
	
	if(ret->data == NULL){
		printf("ciao\n");
		}
	memcpy(ret->data,data,data_len);
	
	return ret;
	}
	
	
void delete_near_addr(const struct sockaddr_in *addr){
	
	struct near_node * iterator = near_list_head;
	while(iterator != NULL){
		
		if(!memcmp(&(iterator->near_addr), addr, sizeof(struct sockaddr_in) )){
			
			near_list_head = remove_near_node(near_list_head,iterator);		
		}
		iterator = iterator->next;
	}
	return;
	
}	

struct near_node *remove_near_node(struct near_node *head, struct near_node *toremove) {
	if (toremove == NULL) {
		return head;
	}
	
	if (toremove->prev == NULL) {
		//primo nodo
		if (toremove->next == NULL) {
			//primo e unico			
			head = NULL;
		} else {
			//primo ma non unico
			toremove->next->prev = NULL;
			head = toremove->next;
		}
	} else {
		//no primo nodo
		if (toremove->next == NULL) {
			//ultimo nodo
			toremove->prev->next = NULL;
		} else {
			//ne primo ne ultimo
			 toremove->prev->next = toremove->next;
			 toremove->next->prev = toremove->prev;
		}
	}
	free(toremove);
	
	toremove = NULL;
	return head;
}

