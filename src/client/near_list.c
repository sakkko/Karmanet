//near list
#include "near_list.h"


int insert_near(int socksd, const struct sockaddr_in *addr){
	struct near_node *iterator;
	struct near_node *to_create;

	if (near_list_head == NULL){
		//creo testa	
		near_list_head = create_new_near_node(socksd, addr);
		near_list_head->next = NULL;
		near_list_head->prev = NULL;
		return 0;
	}

	iterator = near_list_head;
	while(iterator != NULL){
		if (iterator->socksd == socksd) {
			if (!memcmp(&(iterator->addr), addr, sizeof(struct sockaddr_in))) {
				return 0;		
			} else {
				fprintf(stderr, "insert_near error - list is not consistent\n");
				return -1;
			}

		}
		
		iterator = iterator->next;
	}

	//nodo non trovato inserisco in testa
	to_create = create_new_near_node(socksd, addr);
	to_create->next = near_list_head->next;
	to_create->prev = near_list_head;
	
	if (near_list_head->next != NULL) {
		near_list_head->next->prev = to_create;
	}

	near_list_head->next = to_create;
	
	return 0;
}

int update_near(int socksd, const char *data, unsigned int data_len) {
	if (near_list_head == NULL) {
		fprintf(stderr, "update_near error - list is empty\n");
		return -1;
	}

	struct near_node *iterator = near_list_head;

	while (iterator != NULL) {
		if (iterator->socksd == socksd) {
			memcpy(iterator->data, data, data_len);
			iterator->near_number = data_len / ADDR_STR_LEN;
			return 0;		
		}
		iterator = iterator->next;
	}

	if (iterator == NULL) {
		fprintf(stderr, "update_near error - entry not found\n");
		return -1;
	}

	return 0;
}

struct near_node *get_near_node(int socksd) {
	struct near_node * iterator = near_list_head;

	while(iterator != NULL){
		if (iterator->socksd == socksd) {
			break;
		}
		
		iterator = iterator->next;
	}

	return iterator;
	
}

struct near_node *create_new_near_node(int socksd, const struct sockaddr_in *addr){
	int rc;
	struct near_node *ret = (struct near_node *)malloc(sizeof(struct near_node));

	ret->socksd = socksd;
	addrcpy(&ret->addr, addr);

	if ((rc = pthread_mutex_init(&ret->mutex, NULL)) != 0) {
		fprintf(stderr, "create_new_near_node error - can't initialize lock: %s\n", strerror(rc));
		return NULL;
	}
	return ret;
}
	
	
int delete_near(int socksd) {
	struct near_node *iterator = near_list_head;
	int rc;

	while(iterator != NULL){
		if (iterator->socksd == socksd) {
			if ((rc = pthread_mutex_destroy(&iterator->mutex)) != 0) {
				fprintf(stderr, "delete_near error - can't destroy lock: %s\n", strerror(rc));
				return -1;
			}

			near_list_head = remove_near_node(near_list_head, iterator);
			return 0;
		}
		iterator = iterator->next;
	}

	return -1;
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

