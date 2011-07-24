#include "list.h"

/*
* Funzione che inserisce un indirizzo in testa alla lista.
*/
struct node *insert_node(struct node *list, void *data) {
	struct node *new_node;
	
	if (data == NULL) {
		return list;
	}
	
	new_node = (struct node *)malloc(sizeof(struct node));
	new_node->data = data;
	new_node->prev = NULL;
	new_node->next = list;
	
	if (list != NULL) {
		list->prev = new_node;
	} 
	
	return new_node;
}

/*
* Funzione che inserisce un indirizzo in mezzo alla lista.
*/
struct node *sorted_insert_node(struct node *list, struct node *prev_node, void *data) {
	struct node *new_node;
	
	if (data == NULL) {
		return list;
	}

	if (list == NULL || prev_node == NULL) {
		//inserisco in cima
		return insert_node(list, data);
	}
	
	new_node = (struct node *)malloc(sizeof(struct node));
	new_node->data = data;

	//inserisco in mezzo
	new_node->prev = prev_node;
	new_node->next = prev_node->next;
	if (prev_node->next != NULL) {
		//no ultimo elemento 
		prev_node->next->prev = new_node;
	} 
	prev_node->next = new_node;
	return list;
}

/*
* Funzione che rimuove un nodo dalla lista.
*/
struct node *remove_node(struct node *list, struct node *toremove) {
	if (toremove == NULL) {
		return list;
	}
	
	if (toremove->prev == NULL) {
		//primo nodo
		if (toremove->next == NULL) {
			//primo e unico			
			list = NULL;
		} else {
			//primo ma non unico
			toremove->next->prev = NULL;
			list = toremove->next;
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
	return list;
}

/*
* Funzione che elimina l'intera lista
*/
void free_list(struct node *list) {
	struct node *it = list;
	
	while (it != NULL) {
		list = list->next;
		free(it);
		it = list;
	}
	
}

/*
* Funzione che conta gli elementi della lista
*/
int get_list_count(const struct node *head){
	struct node *tmp_node;	
	tmp_node = head;
	int dim = 0;
	while( tmp_node != NULL){
		tmp_node = tmp_node->next;
		dim ++;	
	}
	return dim;
}

