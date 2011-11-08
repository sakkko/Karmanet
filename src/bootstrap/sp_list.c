#include "sp_list.h"

/*
* Funzione che inserisce un indirizzo nella lista.
*/
void insert_sp(const struct sockaddr_in *sp_addr, unsigned char free_spots) {
	struct node *tmp_node;
	struct spaddr_node *new_node;

	if (sp_list_head == NULL) {
		sp_count = 0;
	}
	
	if ((tmp_node = get_node_sp(sp_addr)) == NULL) {
		//indirizzo non presente in lista, lo inserisco
		new_node = (struct spaddr_node *)malloc(sizeof(struct spaddr_node));
		new_node->flag = 1;
		new_node->free_spots = free_spots;
		addrcpy(&new_node->sp_addr, sp_addr);
		sp_list_head = insert_node(sp_list_head, new_node);
		
		if (it_addr == NULL) {
			it_addr = sp_list_head;
		}
		sp_count ++;
	} else {
		((struct spaddr_node *)tmp_node->data)->flag = 1;
	}
}

/*
* Funzione che elimina un indirizzo dalla lista.
*/
void remove_sp(const struct sockaddr_in *sp_addr) {
	struct node *tmp_node;
	
	if ((tmp_node = get_node_sp(sp_addr)) != NULL) {
		if (it_addr == tmp_node) {
			if (tmp_node->next != NULL) {
				it_addr = tmp_node->next;
			} else if (tmp_node->prev != NULL) {
				it_addr = tmp_node->prev;
			} else {
				it_addr = NULL;
			}
		}
		remove_sp_node(tmp_node);
	}
}

/*
* Funzione che rimuove un nodo dalla lista.
*/
void remove_sp_node(struct node *sp_node) {
	if (sp_node == NULL || sp_list_head == NULL) {
		return;
	}
	
	free(sp_node->data);
	sp_node->data = NULL;
	sp_list_head = remove_node(sp_list_head, sp_node);
	sp_count --;
}

/*
* Funzione che ritorna il nodo contenente l'indirizzo passato per parametro.
* Ritorna un puntatore al nodo che contiene l'indirizzo passato per parametro,
* oppure NULL se l'indirizzo non è presente in lista.
*/
struct node *get_node_sp(const struct sockaddr_in *sp_addr) {
	struct node *tmp_node;
	
	tmp_node = sp_list_head;
	
	while (tmp_node != NULL) {
		if (addrcmp(&((struct spaddr_node *)tmp_node->data)->sp_addr, sp_addr)) {
			return tmp_node;
		}
		tmp_node = tmp_node->next;
	}
	
	return NULL;
}

/*
* Funzione che restituisce l'indirizzo puntato dall'iteratore e incrementa l'iteratore
* per puntare al nodo successivo. Ritorna l'indirizzo puntato dall'iteratore.
*/
struct sockaddr_in *get_addr() {
	struct sockaddr_in *ret;
	
	ret = &((struct spaddr_node *)it_addr->data)->sp_addr;
	if (it_addr->next != NULL) {
		it_addr = it_addr->next;
	} else {
		it_addr = sp_list_head;
	}
	
	return ret;
}

/*
* Funzione che elimina l'intera lista
*/
void free_list_sp() {
	while (sp_list_head != NULL) {
		remove_sp_node(sp_list_head);
	}
}

struct node *get_addr_sp(int *dim) {
	struct node *ret, *it;
	struct spaddr_node *sp_node;

	it = sp_list_head;
	*dim = 0;
	ret = NULL;

	while (it != NULL) {
		sp_node = (struct spaddr_node *)it->data;
		if (sp_node->free_spots > 0) {
			ret = insert_node(ret, sp_node);
			(*dim) ++;
			if (*dim >= addr_to_send) {
				break;
			}
		}
		it = it->next;
	}

	if (ret == NULL) {
		fprintf(stderr, "Nessun super-peer libero trovato\n");
	}

	return ret;
}

