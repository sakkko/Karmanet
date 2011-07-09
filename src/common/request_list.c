#include "request_list.h"

/*
* Funzione che inserisce un indirizzo nella lista.
*/
void insert_request(const struct sockaddr_in *addr, short index) {
	struct node *tmp_node;
	struct request_node *new_node;
	
	if ((tmp_node = get_request_node(addr, index)) == NULL) {
		//indirizzo non presente in lista, lo inserisco
		new_node = (struct request_node *)malloc(sizeof(struct request_node));
		new_node->index = index;
		addrcpy(&new_node->addr, addr);
		request_list_head = insert_node(request_list_head, new_node);
		
	}
}

/*
* Funzione che elimina un indirizzo dalla lista.
*/
void remove_request(const struct sockaddr_in *addr, short index) {
	struct node *tmp_node;
	
	if ((tmp_node = get_request_node(addr, index)) != NULL) {
		free(tmp_node->data);
		tmp_node->data = NULL;
		request_list_head = remove_node(request_list_head, tmp_node);
	}
}

/*
* Funzione che ritorna il nodo contenente l'indirizzo passato per parametro.
* Ritorna un puntatore al nodo che contiene l'indirizzo passato per parametro,
* oppure NULL se l'indirizzo non è presente in lista.
*/
struct node *get_request_node(const struct sockaddr_in *addr, short index) {
	struct node *tmp_node;
	
	tmp_node = request_list_head;
	
	while (tmp_node != NULL) {
		//ret = (struct spaddr_node *)tmp_node->data;
		if (addrcmp(&((struct request_node *)tmp_node->data)->addr, addr)) {
			if (((struct request_node *)tmp_node->data)->index == index) {
				return tmp_node;
			}
		}
		tmp_node = tmp_node->next;
	}
	
	return NULL;
}

/*
* Funzione che rimuove un nodo dalla lista.
*/
void remove_request_node(struct node *request_node) {
	if (request_node == NULL || request_list_head == NULL) {
		return;
	}
	
	free(request_node->data);
	request_node->data = NULL;
	request_list_head = remove_node(request_list_head, request_node);
}

/*
* Funzione che elimina l'intera lista
*/
void free_list_request() {
	while (request_list_head != NULL) {
		remove_request_node(request_list_head);
	}
}

