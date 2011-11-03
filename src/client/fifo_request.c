#include "fifo_request.h"

/*
* Funzione che inserisce un indirizzo in testa alla lista.
*/
int insert_request(unsigned short id, unsigned long ip_sender, unsigned short port) { 
	struct node *tmp_node;
	struct request_node *new_node;
	
	if (request_list_head == NULL) {
	//	curr_p_count = 0;
	}
	if ((tmp_node = get_request_node(id, ip_sender, port)) == NULL) {
		//indirizzo non presente in lista, lo inserisco
		new_node = new_request_node(id, ip_sender, port);
		request_list_head = insert_node(request_list_head, new_node);
	//	curr_p_count ++;	
	} else {
		return 1;
	}

	return 0;
}


/*
 * Funzione che crea un nuovo nodo.
 */
struct request_node *new_request_node(unsigned short id, unsigned long ip_sender, unsigned short port) {
	struct request_node *new_node;

	new_node = (struct request_node *)malloc(sizeof(struct request_node));
	new_node->id = id;
	new_node->ip_sender = ip_sender;
	new_node->port = port;
	new_node->ttl = REQUEST_TTL;

	return new_node;
}

/*
* Funzione che elimina un indirizzo dalla lista.
*/
void remove_request(unsigned short id, unsigned long ip_sender, unsigned short port) {
	struct node *tmp_node;
	
	if ((tmp_node = get_request_node(id, ip_sender, port)) != NULL) {
		free(tmp_node->data);
		tmp_node->data = NULL;
		request_list_head = remove_node(request_list_head, tmp_node);
//		curr_p_count --;
	}
}

/*
* Funzione che ritorna il nodo contenente l'indirizzo passato per parametro.
* Ritorna un puntatore al nodo che contiene l'indirizzo passato per parametro,
* oppure NULL se l'indirizzo non è presente in lista.
*/
struct node *get_request_node(unsigned short id, unsigned long ip_sender, unsigned short port) {
	struct node *tmp_node;
	struct request_node *app;

	tmp_node = request_list_head;
	
	while (tmp_node != NULL) {
		app = (struct request_node *)tmp_node->data;
		if (app->id == id && app->ip_sender == ip_sender && app->port == port) {
			return tmp_node;
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
//	curr_p_count --;
//	printf("CURR PEER COUNT = %d\n", curr_p_count);
}

/*
* Funzione che elimina l'intera lista
*/
void free_list_request() {
	while (request_list_head != NULL) {
		remove_request_node(request_list_head);
	}
}

