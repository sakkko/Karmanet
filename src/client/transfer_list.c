#include "transfer_list.h"

void print_file_info(const struct transfer_info *file_info) {
	printf("filename: %s\n", file_info->filename);
	printf("md5: ");
	print_as_hex(file_info->md5, MD5_DIGEST_LENGTH);
	printf("file size: %d\n", file_info->file_size);
	printf("chunk number: %d\n", file_info->chunk_number);
}

void insert_download(const struct sockaddr_in *addr, const unsigned char *md5) {
	struct transfer_node *new_node;
	
	if (download_list_head == NULL) {
		download_count = 0;
	}

	new_node = new_download_node(addr, md5);
	download_list_head = insert_node(download_list_head, new_node);
	download_count ++;	
}


struct transfer_node *new_download_node(const struct sockaddr_in *addr, const unsigned char *md5) {
	struct transfer_node *new_node;

	new_node = (struct transfer_node *)malloc(sizeof(struct transfer_node));
	addrcpy(&new_node->addrto, addr);
	memcpy(new_node->file_info.md5, md5, MD5_DIGEST_LENGTH);

	return new_node;
}

struct transfer_node *get_download() {
	struct transfer_node *ret = NULL;
	struct node *it;


	it = download_list_head;

	while (it != NULL) {
		if (it->next == NULL) {
			break;
		}
	}
	
	if (it == NULL) {
		return NULL;
	}
	
	ret = (struct transfer_node *)it->data;
	download_list_head = remove_node(download_list_head, it);
	download_count --;
	
	return ret;
}

void insert_upload(int socksd) {
	struct transfer_node *new_node;
	
	if (upload_list_head == NULL) {
		upload_count = 0;
	}

	new_node = new_upload_node(socksd);
	upload_list_head = insert_node(upload_list_head, new_node);
	upload_count ++;	
}

struct transfer_node *new_upload_node(int socksd) {
	struct transfer_node *new_node;

	new_node = (struct transfer_node *)malloc(sizeof(struct transfer_node));
	new_node->socksd = socksd;

	return new_node;
}

struct transfer_node *get_upload() {
	struct transfer_node *ret = NULL;
	struct node *it;


	it = upload_list_head;

	while (it != NULL) {
		if (it->next == NULL) {
			break;
		}
	}
	
	if (it == NULL) {
		return NULL;
	}
	
	ret = (struct transfer_node *)it->data;
	upload_list_head = remove_node(upload_list_head, it);
	upload_count --;
	
	return ret;
}

