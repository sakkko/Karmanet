#ifndef _download_LIST_H
#define _download_LIST_H

#include "inetutil.h"
#include "list.h"
#include "md5_util.h"

struct transfer_info {
	char filename[255];
	unsigned char md5[MD5_DIGEST_LENGTH]; 
	unsigned int file_size;
	unsigned int chunk_number;
};

struct transfer_node {
	int socksd;
	struct sockaddr_in addrto;
	struct transfer_info file_info;
};

struct node *download_list_head;
struct node *upload_list_head;

int download_count;
int upload_count;

void insert_download(const struct sockaddr_in *addr, const unsigned char *md5);

struct transfer_node *new_download_node(const struct sockaddr_in *addr, const unsigned char *md5);

struct transfer_node *get_download();

void print_file_info(const struct transfer_info *file_info);

#endif
