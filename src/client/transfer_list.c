#include "transfer_list.h"

void insert_download(const struct sockaddr_in *addr, unsigned char* md5) {
	struct download_node *new_node;
	
	if (download_list_head == NULL) {
		download_count = 0;
	}

	new_node = new_download_node( addr,md5);
	download_list_head = insert_node(download_list_head, new_node);
	download_count ++;	
}


/*
 * Funzione che crea un nuovo nodo.
 */
struct download_node *new_download_node(const struct sockaddr_in *addr, unsigned char* md5) {
	struct download_node *new_node;

	new_node = (struct download_node *)malloc(sizeof(struct download_node));
	addrcpy(&new_node->addrto, addr);
	memcpy(new_node->md5,md5,MD5_DIGEST_LENGTH);

	return new_node;
}

/*
* Funzione che elimina un indirizzo dalla lista.
*/
struct download_node *pop_download() {
	struct node *tmp_node;
	struct download_node *ret = NULL;


	if (download_list_head == NULL) {
		return NULL;
	}
	ret = (struct download_node *)download_list_head->data;
	download_list_head = remove_node(download_list_head, download_list_head);
	
	return ret;
}



