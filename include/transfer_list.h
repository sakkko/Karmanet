#ifndef _download_LIST_H
#define _download_LIST_H

#include "download_util.h"
#include "list.h"

struct download_node {
	struct sockaddr_in addrto;
	unsigned char  md5[MD5_DIGEST_LENGTH] 
};

struct node *download_list_head;

int download_count;

void insert_download( const struct sockaddr_in *addr, unsigned char* md5);

struct download_node *new_download_node( const struct sockaddr_in *addr, unsigned char* md5);

struct download_node *pop_download();

void free_download_list();

#endif
