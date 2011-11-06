#include "uploader.h"

void uploader_func(void *args) {
	struct uploader_info *ulinfo = (struct uploader_info *)args;
	struct packet send_pck, recv_pck;
	int fd;
	int rc;
	char buf[MAXLINE];
	char str[16];
	int i;

	while (1) {
		if ((rc = pthread_mutex_lock(&ulinfo->th_mutex)) != 0) {
			fprintf(stderr, "uploader_func error - can't release lock: %s\n", strerror(rc));
			pthread_exit((void *)-1);
		}

		if ((rc = pthread_cond_wait(&ulinfo->th_cond, &ulinfo->th_mutex)) != 0) {
			fprintf(stderr, "uploader_func error - can't release lock: %s\n", strerror(rc));
			pthread_exit((void *)-1);
		}

		if ((rc = pthread_mutex_unlock(&ulinfo->th_mutex)) != 0) {
			fprintf(stderr, "uploader_func error - can't release lock: %s\n", strerror(rc));
			pthread_exit((void *)-1);
		}

		sprintf(str, "GETU %d", ulinfo->index);

		if ((i = recv_packet_tcp(ulinfo->ulnode.socksd, &recv_pck)) < 0) {
			fprintf(stderr, "uploader_func errror - recv_packet_tcp failed\n");
			if (close_sock(ulinfo->ulnode.socksd) < 0) {
				perror("uploader_func error - close_sock failed");
			}
			if (write_err(ulinfo->th_pipe, ulinfo->pipe_mutex, str) < 0) {
				fprintf(stderr, "uploader_func error - write_err failed\n");
			}
			continue;
		} else if (i == 0) {
			fprintf(stderr, "uploader_func errror - connection closed by peer\n");
			if (close_sock(ulinfo->ulnode.socksd) < 0) {
				perror("uploader_func error - close_sock failed");
			}
			if (write_err(ulinfo->th_pipe, ulinfo->pipe_mutex, str) < 0) {
				fprintf(stderr, "uploader_func error - write_err failed\n");
			}
			continue;
		}
		
		printf("Ricevuto get\n");
		if (strncmp(recv_pck.cmd, CMD_GET, CMD_STR_LEN)) {
			fprintf(stderr, "packet not expected\n");
			if (close_sock(ulinfo->ulnode.socksd) < 0) {
				perror("uploader_func error - close_sock failed");
			}
			if (write_err(ulinfo->th_pipe, ulinfo->pipe_mutex, str) < 0) {
				fprintf(stderr, "uploader_func error - write_err failed\n");
			}
			continue;
		}

		if (recv_pck.data_len != MD5_DIGEST_LENGTH) {
			fprintf(stderr, "bad packet format\n");
			if (close_sock(ulinfo->ulnode.socksd) < 0) {
				perror("uploader_func error - close_sock failed");
			}
			if (write_err(ulinfo->th_pipe, ulinfo->pipe_mutex, str) < 0) {
				fprintf(stderr, "uploader_func error - write_err failed\n");
			}
			continue;
		}


		if ((i = get_filepath(buf, ulinfo->ulnode.file_info.filename, (const unsigned char *)recv_pck.data)) < 0) {
			fprintf(stderr, "uploader_func error - get_filepath failed\n");
			if (close_sock(ulinfo->ulnode.socksd) < 0) {
				perror("uploader_func error - close_sock failed");
			}
			if (write_err(ulinfo->th_pipe, ulinfo->pipe_mutex, str) < 0) {
				fprintf(stderr, "uploader_func error - write_err failed\n");
			}
			continue;
		} else if (i == 1) {
			fprintf(stderr, "uploader_func error - file richiesto non presente\n");
			if (close_sock(ulinfo->ulnode.socksd) < 0) {
				perror("uploader_func error - close_sock failed");
			}

			if (write_err(ulinfo->th_pipe, ulinfo->pipe_mutex, str) < 0) {
				fprintf(stderr, "uploader_func error - write_err failed\n");
			}
			continue;
		}


		if ((fd = open(buf, O_RDONLY)) < 0) {
			perror("uploader_func error - open failed\n");
			if (close_sock(ulinfo->ulnode.socksd) < 0) {
				perror("uploader_func error - close_sock failed");
			}
			if (write_err(ulinfo->th_pipe, ulinfo->pipe_mutex, str) < 0) {
				fprintf(stderr, "uploader_func error - write_err failed\n");
			}
			continue;

		}

		new_get_packet(&send_pck, 0, NULL, 0, 1);
		get_file_info(fd, &ulinfo->ulnode);
		send_pck.data_len = create_infofile_str(send_pck.data, (const unsigned char *)recv_pck.data, &ulinfo->ulnode);
		memcpy(ulinfo->ulnode.file_info.md5, recv_pck.data, MD5_DIGEST_LENGTH);
		print_file_info(&ulinfo->ulnode.file_info);

		if (send_packet_tcp(ulinfo->ulnode.socksd, &send_pck) < 0) {
			fprintf(stderr, "uploader_func error - send_packet_tcp failed\n");
			if (close_sock(ulinfo->ulnode.socksd) < 0) {
				perror("uploader_func error - close_sock failed");
			}
			if (write_err(ulinfo->th_pipe, ulinfo->pipe_mutex, str) < 0) {
				fprintf(stderr, "uploader_func error - write_err failed\n");
			}
			if (close(fd) < 0) {
				perror("uploader_func error - close failed");
			}
			continue;
		}

		while ((i = recv_packet_tcp(ulinfo->ulnode.socksd, &recv_pck)) > 0) {
			if (strncmp(recv_pck.cmd, CMD_GET, CMD_STR_LEN)) {
				fprintf(stderr, "uploader_func error - packet not expected\n");
				if (close_sock(ulinfo->ulnode.socksd) < 0) {
					perror("uploader_func error - close_sock failed");
				}
				if (close(fd) < 0) {
					perror("uploader_func error - close failed");
				}
				if (write_err(ulinfo->th_pipe, ulinfo->pipe_mutex, str) < 0) {
					fprintf(stderr, "uploader_func error - write_err failed\n");
				}
				continue;
			}

			if (send_chunk(fd, recv_pck.index, &ulinfo->ulnode) < 0) {
				fprintf(stderr, "uploader_func error - send_chunk failed\n");
				if (close_sock(ulinfo->ulnode.socksd) < 0) {
					perror("uploader_func error - close_sock failed");
				}
				if (close(fd) < 0) {
					perror("uploader_func error - close failed");
				}
				if (write_err(ulinfo->th_pipe, ulinfo->pipe_mutex, str) < 0) {
					fprintf(stderr, "uploader_func error - write_err failed\n");
				}
				continue;
			}
		}

		if (i < 0) {
			fprintf(stderr, "uploader_func error - recv_packet_tcp failed\n");
		}

		if (close_sock(ulinfo->ulnode.socksd) < 0) {
			perror("uploader_func error - close_sock failed");
		}
		if (close(fd) < 0) {
			perror("uploader_func error - close failed");
		}
		
		sprintf(str, "TRM %d\n", ulinfo->index);
		write_on_pipe(ulinfo->th_pipe, ulinfo->pipe_mutex, str);
	}

}

void get_file_info(int fd, struct transfer_node *ulnode) {
	ulnode->file_info.file_size = get_file_size(fd);
	ulnode->file_info.chunk_number = ulnode->file_info.file_size / CHUNK_SIZE;
	if (ulnode->file_info.file_size % CHUNK_SIZE) {
		ulnode->file_info.chunk_number ++;
	}

}

int send_chunk(int fd, int chunk_index, const struct transfer_node *ulnode) {
	struct packet send_pck;
	int j, nread;
	int part_number;

	new_packet(&send_pck, CMD_DATA, chunk_index, NULL, 0, 1); 
	lseek(fd, chunk_index * CHUNK_SIZE, SEEK_SET);
	part_number = get_part_number(chunk_index, ulnode);

	printf("Invio chunk %d\n", chunk_index);
	for (j = 0; j < part_number; j ++) {
		if ((nread = read(fd, send_pck.data, MAXLINE)) < 0) {
			fprintf(stderr, "uploader_func error - read failed\n");
			return -1;
		} else if (nread == 0) {
			fprintf(stderr, "send_part error - invalid part number\n");
			return -1;
		}

		if (j == part_number - 1) {
			unset_nextchunk_flag(&send_pck);
		} else {
			set_nextchunk_flag(&send_pck);
			if (nread != MAXLINE) {
				fprintf(stderr, "uploader_func error - internal error\n");
				return -1;
			}
		}

		send_pck.data_len = nread;
		if (send_packet_tcp(ulnode->socksd, &send_pck) < 0) {
			fprintf(stderr, "uploader_func error - send_packet_tcp failed\n");
			return -1;
		}
	}

	return 0;
}

int get_part_number(int chunk_number, const struct transfer_node *ulnode) {
	int part;

	if (chunk_number == ulnode->file_info.chunk_number - 1) {
		part = (ulnode->file_info.file_size - chunk_number * CHUNK_SIZE) / MAXLINE;
		if ((ulnode->file_info.file_size - chunk_number * CHUNK_SIZE) % MAXLINE) {
			part ++;
		}
	} else {
		part = CHUNK_SIZE / MAXLINE;
	}

	return part;
}

int create_infofile_str(char *data, const unsigned char *md5, const struct transfer_node *ulnode) {
	int offset;

	strcpy(data, ulnode->file_info.filename);
	offset = strlen(ulnode->file_info.filename);
	data[offset] = 0;
	offset ++;

	memcpy(data + offset, md5, MD5_DIGEST_LENGTH);
	offset += MD5_DIGEST_LENGTH;
	memcpy(data + offset, (char *)&ulnode->file_info.file_size, sizeof(ulnode->file_info.file_size));
	offset += sizeof(ulnode->file_info.file_size);
	memcpy(data + offset, (char *)&ulnode->file_info.chunk_number, sizeof(ulnode->file_info.chunk_number));
	offset += sizeof(ulnode->file_info.chunk_number);

	return offset;
}

int uploader_init(int nthread, int th_pipe, pthread_mutex_t *mutex) {
	int i, rc;

	if (ul_pool != NULL) {
		return 0;
	}

	ul_pool = (struct uploader_info *)malloc(sizeof(struct uploader_info) * nthread);
	for (i = 0; i < nthread; i ++) {
		ul_pool[i].th_pipe = th_pipe;
		ul_pool[i].pipe_mutex = mutex;
		if ((rc = pthread_cond_init(&ul_pool[i].th_cond, NULL)) != 0) {
			fprintf(stderr, "uploader_init error - pthread_cond_init failed: %s\n", strerror(rc));
			return -1;
		}
		if ((rc = pthread_mutex_init(&ul_pool[i].th_mutex, NULL)) != 0) {
			fprintf(stderr, "uploader_init error - pthread_mutex_init failed: %s\n", strerror(rc));
			return -1;
		}
		ul_pool[i].sleeping = 1;
		ul_pool[i].index = i;
		if (uploader_run(&ul_pool[i]) < 0) {
			fprintf(stderr, "uploader_init error - uploader_run failed\n");
			return -1;
		}
	}

	return 0;
}

int uploader_run(struct uploader_info *ulinfo) {
	int rc;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	if ((rc = pthread_create(&ulinfo->thread, &attr, (void *)uploader_func, (void *)ulinfo)) != 0) {
		fprintf(stderr, "uploader_run error - can't create thread: %s\n", strerror(rc));
		return -1;
	}

	return 0;
}



