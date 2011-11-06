#ifndef _DOWNLOADER_H
#define _DOWNLOADER_H

#include "transfer.h"

struct downloader_info {
	pthread_t thread;
	pthread_mutex_t *pipe_mutex;
	int th_pipe;
};

struct downloader_info *dw_pool;

int downloader_init(int nthread, int th_pipe, pthread_mutex_t *pipe_mutex);

int downloader_run(struct downloader_info *dwinfo);

void downloader_func(void *args);

int add_download(const struct sockaddr_in *addr, const unsigned char *md5);

int *get_missing_chunk(int fdpart, int chunk_number, int my_chunk_number, int miss_chunk_number);

int create_file_part(const char *partname, const struct transfer_node *dwnode, int *miss_chunk_number, int *my_chunk_number);

int open_connection(struct transfer_node *dwnode);

int fill_file_info(struct transfer_info *file_info, const struct packet *recv_pck);

int kill_connection(struct downloader_info *dwinfo, struct transfer_node *dwnode);

int write_filepart(int fdpart, int missing_chunk, int my_chunk_number, int filename_len);

int get_chunk(int fd, int fdpart, int socksd, int missing_chunk);

int open_file_part(char *partname, struct transfer_node *dwnode, int *miss_chunk_number, int *my_chunk_number);

int load_file_part(int fdpart, const char *partname, struct transfer_node *dwnode, int *miss_chunk_number, int *my_chunk_number);

int download(int fd, int fdpart, const char *partname, const struct transfer_node *dwnode, const int *missing_chunk, int miss_chunk_number, int my_chunk_number);

int get_filepart_name(char *partname, const struct transfer_node *dwnode);

int remove_filepart(const char *partname, const struct transfer_node *dwnode);

#endif
