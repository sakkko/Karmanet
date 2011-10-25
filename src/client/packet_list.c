#include "packet_list.h"

void insert_packet(int socksd, const struct packet *pck, const struct sockaddr_in *addr, short nretx, short countdown, short errors) {
	struct packet_node *new_node;
	
	if (packet_list_head == NULL) {
		packet_count = 0;
	}

	new_node = new_packet_node(socksd, pck, addr, nretx, countdown, errors);
	packet_list_head = insert_node(packet_list_head, new_node);
	packet_count ++;	
}


/*
 * Funzione che crea un nuovo nodo.
 */
struct packet_node *new_packet_node(int socksd, const struct packet *pck, const struct sockaddr_in *addr, short nretx, short countdown, short errors) {
	struct packet_node *new_node;

	new_node = (struct packet_node *)malloc(sizeof(struct packet_node));
	new_node->socksd = socksd;
	new_node->nretx = nretx;
	new_node->errors = errors;
	new_node->countdown = countdown;
	addrcpy(&new_node->addrto, addr);
	pckcpy(&new_node->pck, pck);

	return new_node;
}

/*
* Funzione che elimina un indirizzo dalla lista.
*/
void remove_packet(unsigned short pck_index) {
	struct node *tmp_node;
	
	if ((tmp_node = get_packet_node(pck_index)) != NULL) {
		free(tmp_node->data);
		tmp_node->data = NULL;
		packet_list_head = remove_node(packet_list_head, tmp_node);
		packet_count --;
	}
}

/*
* Funzione che ritorna il nodo contenente l'indirizzo passato per parametro.
* Ritorna un puntatore al nodo che contiene l'indirizzo passato per parametro,
* oppure NULL se l'indirizzo non è presente in lista.
*/
struct node *get_packet_node(unsigned short pck_index) {
	struct node *tmp_node;
	
	tmp_node = packet_list_head;
	
	while (tmp_node != NULL) {
		//ret = (struct spaddr_node *)tmp_node->data;
		if (((struct packet_node *)tmp_node->data)->pck.index == pck_index) {
			return tmp_node;
		}
		tmp_node = tmp_node->next;
	}
	
	return NULL;
}

/*
* Funzione che rimuove un nodo dalla lista.
*/
void remove_packet_node(struct node *packet_node) {
	if (packet_node == NULL || packet_list_head == NULL) {
		return;
	}
	
	free(packet_node->data);
	packet_node->data = NULL;
	packet_list_head = remove_node(packet_list_head, packet_node);
	packet_count --;
}

/*
* Funzione che elimina l'intera lista
*/
void free_packet_list() {
	while (packet_list_head != NULL) {
		remove_packet_node(packet_list_head);
	}
}

