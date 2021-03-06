#ifndef _UPLOADER_H
#define _UPLOADER_H

#include "transfer.h"

struct uploader_info {
	pthread_t thread;
	pthread_mutex_t *pipe_mutex;
	pthread_mutex_t th_mutex;
	pthread_cond_t th_cond;
	struct transfer_node ulnode;
	int th_pipe;
	int sleeping;
	int index;
};

struct uploader_info *ul_pool;

int uploader_init(int nthread, int th_pipe, pthread_mutex_t *pipe_mutex);

int uploader_run(struct uploader_info *ulinfo);

void uploader_func(void *args);

int create_infofile_str(char *data, const unsigned char *md5, const struct transfer_node *ulnode);

int send_chunk(int fd, int chunk_index, const struct transfer_node *ulnode);

int get_part_number(int chunk_number, const struct transfer_node *ulnode);

void get_file_info(int fd, struct transfer_node *ulnode);

#endif
