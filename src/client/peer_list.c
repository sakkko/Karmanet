#include "peer_list.h"

/*
* Funzione che inserisce un indirizzo in testa alla lista.
*/
void insert_peer(const struct sockaddr_in *peer_addr, unsigned long peer_rate) {
	struct node *tmp_node;
	struct peer_node *new_node;
	
	if (peer_list_head == NULL) {
		curr_p_count = 0;
	}
	if ((tmp_node = get_node_peer(peer_addr)) == NULL) {
		//indirizzo non presente in lista, lo inserisco
		new_node = new_peer_node(peer_addr, peer_rate);
		peer_list_head = insert_node(peer_list_head, new_node);
		curr_p_count ++;	
	} else {
		((struct peer_node *)tmp_node->data)->flag = 1;
	}
}

/*
 * Funzione che inserisce un indirizzo nella lista, mantenendo la lista ordinata
 * secondo il rate del peer.
 */
void sorted_insert_peer(const struct sockaddr_in *peer_addr, unsigned long peer_rate) {
	struct node *tmp_node;
	struct peer_node *tmp_peernode;

	if (peer_list_head == NULL) {
		curr_p_count = 0;
	}

	tmp_node = peer_list_head;

	while (tmp_node != NULL) {
		tmp_peernode = (struct peer_node *)tmp_node->data;
		if (addrcmp(peer_addr, &tmp_peernode->peer_addr)) {
			return;
		}
		if (peer_rate > tmp_peernode->peer_rate) {
			//passa alla funzione di inserimento il nodo precedente
			tmp_node = tmp_node->prev;
			break;
		}
		if (tmp_node->next == NULL) {
			break;	
		}
		tmp_node = tmp_node->next;
	}

 	peer_list_head = sorted_insert_node(peer_list_head, tmp_node, new_peer_node(peer_addr, peer_rate));
	curr_p_count ++;
}

/*
 * Funzione che crea un nuovo nodo.
 */
struct peer_node *new_peer_node(const struct sockaddr_in *peer_addr, unsigned long peer_rate) {
	struct peer_node *new_node;

	new_node = (struct peer_node *)malloc(sizeof(struct peer_node));
	new_node->flag = 1;
	new_node->peer_rate = peer_rate;
	addrcpy(&new_node->peer_addr, peer_addr);

	return new_node;
}

/*
* Funzione che elimina un indirizzo dalla lista.
*/
void remove_peer(const struct sockaddr_in *peer_addr) {
	struct node *tmp_node;
	
	if ((tmp_node = get_node_peer(peer_addr)) != NULL) {
		free(tmp_node->data);
		tmp_node->data = NULL;
		peer_list_head = remove_node(peer_list_head, tmp_node);
		curr_p_count --;
	}
}

/*
* Funzione che ritorna il nodo contenente l'indirizzo passato per parametro.
* Ritorna un puntatore al nodo che contiene l'indirizzo passato per parametro,
* oppure NULL se l'indirizzo non è presente in lista.
*/
struct node *get_node_peer(const struct sockaddr_in *peer_addr) {
	struct node *tmp_node;
	
	tmp_node = peer_list_head;
	
	while (tmp_node != NULL) {
		//ret = (struct spaddr_node *)tmp_node->data;
		if (addrcmp(&((struct peer_node *)tmp_node->data)->peer_addr, peer_addr)) {
			return tmp_node;
		}
		tmp_node = tmp_node->next;
	}
	
	return NULL;
}

/*
* Funzione che rimuove un nodo dalla lista.
*/
void remove_peer_node(struct node *peer_node) {
	if (peer_node == NULL || peer_list_head == NULL) {
		return;
	}
	
	free(peer_node->data);
	peer_node->data = NULL;
	peer_list_head = remove_node(peer_list_head, peer_node);
	curr_p_count --;
	printf("CURR PEER COUNT = %d\n", curr_p_count);
}

/*
* Funzione che elimina l'intera lista
*/
void free_list_peer() {
	while (peer_list_head != NULL) {
		remove_peer_node(peer_list_head);
	}
}

/*
 * Restituisce l'indirizzo in testa alla lista.
 * Se tutti gli inserimenti nella lista sono ordinati, restituisce 
 * l'indirizzo con il rate maggiore.
 * Restituiesce l'indirizzo in testa alla lista, oppure NULL se la lista è vuota.
 */
struct sockaddr_in *get_addr_head() {
	if (peer_list_head == NULL) {
		return NULL;
	}
	if (peer_list_head->data == NULL) {
		return NULL;
	}

	return &((struct peer_node *)peer_list_head->data)->peer_addr;
}


