#include "downloader.h"

pthread_mutex_t download_list_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t downloads_file_mutex = PTHREAD_MUTEX_INITIALIZER;

void downloader_func(void *args) {
	struct downloader_info *dwinfo = (struct downloader_info *)args;
	struct packet send_pck, recv_pck;
	int fdpart, fd;
	struct timespec sleep_time;
	struct transfer_node *dwnode;
	int i, rc;
	char buf[MAXLINE];
	char partname[262];
	int my_chunk_number, miss_chunk_number;
	int *missing_chunk;

	sleep_time.tv_sec = TIME_TO_SLEEP;
	sleep_time.tv_nsec = 0;

	while (1) {
		nanosleep(&sleep_time, NULL);
		if ((rc = pthread_mutex_lock(&download_list_mutex)) != 0) {
			fprintf(stderr, "retx_func error - can't acquire lock: %s\n", strerror(rc));
			pthread_exit((void *)-1);
		}

		dwnode = get_download();
		
		if ((rc = pthread_mutex_unlock(&download_list_mutex)) != 0) {
			fprintf(stderr, "retx_func error - can't release lock: %s\n", strerror(rc));
			pthread_exit((void *)-1);
		}

		if (dwnode == NULL) {
			continue;
		}
		
		if (open_connection(dwnode) < 0) {
			fprintf(stderr, "downloader_func error - open_connection failed\n");
			kill_connection(dwinfo, dwnode);
			free(dwnode);
			dwnode = NULL;
			continue;
		}

		printf("Connessione accettata\n");

		new_get_packet(&send_pck, 0, (char *)dwnode->file_info.md5, MD5_DIGEST_LENGTH, 0);
		if (send_packet_tcp(dwnode->socksd, &send_pck) < 0 ){
			fprintf(stderr,"downloader_func error - send_packet_tcp failed");	
			kill_connection(dwinfo, dwnode);
			free(dwnode);
			dwnode = NULL;
			continue; 
		}
		
		if ((i = recv_packet_tcp(dwnode->socksd, &recv_pck)) < 0) {
			fprintf(stderr, "downloader_func error - rcv_packet_tcp failed\n");
			kill_connection(dwinfo, dwnode);
			free(dwnode);
			dwnode = NULL;
			continue; 
		} else if (i == 0) {
			printf("Connection close by peer\n");
			kill_connection(dwinfo, dwnode);
			free(dwnode);
			dwnode = NULL;
			continue;
		}

		if (strncmp(recv_pck.cmd, CMD_GET, CMD_STR_LEN)) {
			fprintf(stderr, "packet not expected\n");
			kill_connection(dwinfo, dwnode);
			free(dwnode);
			dwnode = NULL;
			continue; 
		}

		if (fill_file_info(&dwnode->file_info, &recv_pck) < 0) {
			fprintf(stderr, "downloader_func error - fill_file_info failed\n");
			kill_connection(dwinfo, dwnode);
			free(dwnode);
			dwnode = NULL;
			continue; 
		}

		print_file_info(&dwnode->file_info);

		if ((fdpart = open_file_part(partname, dwnode, &miss_chunk_number, &my_chunk_number)) < 0) {
			fprintf(stderr, "downloader_func error - create_file_part failed\n");
			kill_connection(dwinfo, dwnode);
			free(dwnode);
			dwnode = NULL;
			continue; 
		}
		
		printf("File-part name: %s\n", partname);
		printf("Chunk posseduti: %d\n", my_chunk_number);
		if ((missing_chunk = get_missing_chunk(fdpart, dwnode->file_info.chunk_number, my_chunk_number, miss_chunk_number)) == NULL) {
			fprintf(stderr, "downloader_func error - get_missing_chunk failed\n");
			if (close(fdpart) < 0) {
				perror("donwloader_fun error - close failed");
			}
			kill_connection(dwinfo, dwnode);
			free(dwnode);
			dwnode = NULL;
			continue; 
		}
	
		sprintf(buf, "%s/%s", DOWNLOAD_DIR, dwnode->file_info.filename);
		if ((fd = open(buf, O_WRONLY | O_CREAT, 0644)) < 0) {
			perror("donwloader_fun error - open failed");
			if (close(fdpart) < 0) {
				perror("donwloader_fun error - close failed");
			}
			kill_connection(dwinfo, dwnode);
			free(dwnode);
			dwnode = NULL;
			continue; 
		}

		if (download(fd, fdpart, partname, dwnode, missing_chunk, miss_chunk_number, my_chunk_number) < 0) {
			fprintf(stderr, "downloader_func error - download failed\n");
			if (close(fdpart) < 0 || close(fd) < 0) {
				perror("downlaoder_func error - close failed");
			}
			kill_connection(dwinfo, dwnode);
			free(dwnode);
			dwnode = NULL;
			continue;
		}


		if (close_sock(dwnode->socksd) < 0) {
			perror("downloader_func erroe - close_sock failed");
		}	

		free(dwnode);
		dwnode = NULL;
	}

}

int download(int fd, int fdpart, const char *partname, const struct transfer_node *dwnode, const int *missing_chunk, int miss_chunk_number, int my_chunk_number) {
	int i;

	printf("Iniziato download del file %s\n", dwnode->file_info.filename);

	for (i = 0; i < miss_chunk_number; i ++) {
		if (get_chunk(fd, fdpart, dwnode->socksd, missing_chunk[i]) < 0) {
			fprintf(stderr, "download error - get_chunk failed\n");
			return -1;
		}

		my_chunk_number ++;
		if (write_filepart(fdpart, missing_chunk[i], my_chunk_number, strlen(dwnode->file_info.filename)) < 0) {
			fprintf(stderr, "downloader_func error - write_filepart failed\n");
			return -1;
		}
	}


	if (lseek(fd, 0, SEEK_END) == dwnode->file_info.file_size && my_chunk_number == dwnode->file_info.chunk_number) {
		printf("Completato download del file %s\n", dwnode->file_info.filename);

		if (remove_filepart(partname, dwnode) < 0) {
			fprintf(stderr, "downloader_func error - open failed");
			return -1;
		}
	}

	return 0;
}

int remove_filepart(const char *partname, const struct transfer_node *dwnode) {
	int fddownloads, fdtmp;
	int nread, rc;
	char buf[262];
	unsigned char md5[MD5_DIGEST_LENGTH];

	if (unlink(partname) < 0) {
		return -1;
	}


	if ((rc = pthread_mutex_lock(&downloads_file_mutex)) != 0) {
		fprintf(stderr, "remove_filepart error - can't acquire lock: %s\n", strerror(rc));
		return -1;
	}

	if ((fddownloads = open(DOWNLOADS_FILE, O_RDONLY)) < 0) {
		perror("downloader_func error - open failed");
		if ((rc = pthread_mutex_unlock(&downloads_file_mutex)) != 0) {
			fprintf(stderr, "remove_filepart error - can't release lock: %s\n", strerror(rc));
		}
		return -1;
	}

	if ((fdtmp = open("downloads.tmp", O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
		perror("downloader_func error - open failed");
		if (close(fddownloads) < 0) {
			perror("downloader_func error - close failed");
		}
		if ((rc = pthread_mutex_unlock(&downloads_file_mutex)) != 0) {
			fprintf(stderr, "remove_filepart error - can't release lock: %s\n", strerror(rc));
		}
		return -1;
	}

	while ((nread = read(fddownloads, md5, MD5_DIGEST_LENGTH)) > 0) {
		if (nread != MD5_DIGEST_LENGTH) {
			fprintf(stderr, "open_file_part error - bad file format\n");
			if (close(fddownloads) < 0) {
				perror("downloader_func error - close failed");
			}
			if (close(fdtmp) < 0) {
				perror("downloader_func error - close failed");
			}
			if ((rc = pthread_mutex_unlock(&downloads_file_mutex)) != 0) {
				fprintf(stderr, "remove_filepart error - can't release lock: %s\n", strerror(rc));
			}
			return -1;
		}

		if ((nread = readline(fddownloads, buf, 261)) < 0) {
			perror("downloader_func error - readline failed");
			if (close(fddownloads) < 0) {
				perror("downloader_func error - close failed");
			}
			if (close(fdtmp) < 0) {
				perror("downloader_func error - close failed");
			}
			if ((rc = pthread_mutex_unlock(&downloads_file_mutex)) != 0) {
				fprintf(stderr, "remove_filepart error - can't release lock: %s\n", strerror(rc));
			}
			return -1;
		} else if (nread == 0) {
			fprintf(stderr, "open_file_part error - bad file format\n");
			if (close(fddownloads) < 0) {
				perror("downloader_func error - close failed");
			}
			if (close(fdtmp) < 0) {
				perror("downloader_func error - close failed");
			}
			if ((rc = pthread_mutex_unlock(&downloads_file_mutex)) != 0) {
				fprintf(stderr, "remove_filepart error - can't release lock: %s\n", strerror(rc));
			}
			return -1;
		}

		if (memcmp(md5, dwnode->file_info.md5, MD5_DIGEST_LENGTH)) {
			if (write(fdtmp, md5, MD5_DIGEST_LENGTH) < 0) {
				perror("downloader_func error - write failed");
				if (close(fddownloads) < 0) {
					perror("downloader_func error - close failed");
				}
				if (close(fdtmp) < 0) {
					perror("downloader_func error - close failed");
				}
				if ((rc = pthread_mutex_unlock(&downloads_file_mutex)) != 0) {
					fprintf(stderr, "remove_filepart error - can't release lock: %s\n", strerror(rc));
				}
				return -1;
			}

			if (write(fdtmp, buf, nread) < 0) {
				perror("downloader_func error - write failed");
				if (close(fddownloads) < 0) {
					perror("downloader_func error - close failed");
				}
				if (close(fdtmp) < 0) {
					perror("downloader_func error - close failed");
				}
				if ((rc = pthread_mutex_unlock(&downloads_file_mutex)) != 0) {
					fprintf(stderr, "remove_filepart error - can't release lock: %s\n", strerror(rc));
				}
				return -1;
			}
		}
	}

	if (nread < 0) {
		perror("downloader_func error - read failed");
		if (close(fddownloads) < 0) {
			perror("downloader_func error - close failed");
		}
		if (close(fdtmp) < 0) {
			perror("downloader_func error - close failed");
		}
		if ((rc = pthread_mutex_unlock(&downloads_file_mutex)) != 0) {
			fprintf(stderr, "remove_filepart error - can't release lock: %s\n", strerror(rc));
		}
		return -1;
	}

	if (close(fddownloads) < 0) {
		perror("downloader_func error - close failed");
		if (close(fdtmp) < 0) {
			perror("downloader_func error - close failed");
		}
		if ((rc = pthread_mutex_unlock(&downloads_file_mutex)) != 0) {
			fprintf(stderr, "remove_filepart error - can't release lock: %s\n", strerror(rc));
		}
		return -1;
	}

	if (close(fdtmp) < 0) {
		perror("downloader_func error - close failed");
		if ((rc = pthread_mutex_unlock(&downloads_file_mutex)) != 0) {
			fprintf(stderr, "remove_filepart error - can't release lock: %s\n", strerror(rc));
		}
		return -1;
	}

	if (unlink(DOWNLOADS_FILE) < 0) {
		perror("downloader_func error - unlink failed");
		if ((rc = pthread_mutex_unlock(&downloads_file_mutex)) != 0) {
			fprintf(stderr, "remove_filepart error - can't release lock: %s\n", strerror(rc));
		}
		return -1;
	}

	if (rename("downloads.tmp", DOWNLOADS_FILE) < 0) {
		perror("downloader_func error - rename failed");
		if ((rc = pthread_mutex_unlock(&downloads_file_mutex)) != 0) {
			fprintf(stderr, "remove_filepart error - can't release lock: %s\n", strerror(rc));
		}
		return -1;
	}

	if ((rc = pthread_mutex_unlock(&downloads_file_mutex)) != 0) {
		fprintf(stderr, "remove_filepart error - can't release lock: %s\n", strerror(rc));
		return -1;
	}

	return 0;
}

int write_filepart(int fdpart, int missing_chunk, int my_chunk_number, int filename_len) {
	lseek(fdpart, filename_len + MD5_DIGEST_LENGTH + 2 * sizeof(int) + 1, SEEK_SET);
	if (write(fdpart, (char *)&my_chunk_number, sizeof(int)) < 0) {
		perror("write_filepart error - write failed");
		return -1;
	}
	lseek(fdpart, 0, SEEK_END);
	if (write(fdpart, (char *)&missing_chunk, sizeof(int)) < 0) {
		perror("write_filepart error - write failed");
		return -1;
	}

	return 0;
}

int get_chunk(int fd, int fdpart, int socksd, int missing_chunk) {
	struct packet send_pck, recv_pck;
	int n;

	new_get_packet(&send_pck, missing_chunk, NULL, 0, 1); 
	if (send_packet_tcp(socksd, &send_pck) < 0) {
		fprintf(stderr, "get_chunk error - send_packet_tcp failed\n");
		return -1;
	}


	lseek(fd, missing_chunk * CHUNK_SIZE, SEEK_SET);
	while ((n = recv_packet_tcp(socksd, &recv_pck)) > 0) {
		if (strncmp(recv_pck.cmd, CMD_DATA, CMD_STR_LEN)) {
			printf("get_chunk error - packet not expected\n");
			return -1;
		}

		if (recv_pck.index != missing_chunk) {
			printf("get_chunk error - invalid chunk number\n");
			return -1;
		}

		if (write(fd, recv_pck.data, recv_pck.data_len) < 0) {
			perror("get_chunk error - write failed");
			return -1;
		}

		if (!is_set_flag(&recv_pck, PACKET_FLAG_NEXT_CHUNK)) {
//			printf("Chunk %d completato\n", missing_chunk);
			return 0;
		}
	}

	return -1;
}

int kill_connection(struct downloader_info *dwinfo, struct transfer_node *dwnode) {
	if (write_err(dwinfo->th_pipe, dwinfo->pipe_mutex, "GET") < 0) {
		fprintf(stderr, "kill_connection error - write_err failed\n");
		return -1;
	}

	if (close_sock(dwnode->socksd) < 0) {
		perror("kill_connection error - close_sock failed");
		return -1;
	}

	dwnode->socksd = -1;

	return 0;
}


int fill_file_info(struct transfer_info *file_info, const struct packet *recv_pck) {
	int offset;

	strcpy(file_info->filename, recv_pck->data);
	offset = strlen(recv_pck->data);
	offset ++;
	if (memcmp(file_info->md5, recv_pck->data + offset, MD5_DIGEST_LENGTH)) {
		fprintf(stderr, "fill_file_info error - md5 doesn't match\n");
		return -1;
	}
	offset += MD5_DIGEST_LENGTH;
	file_info->file_size = *((int *)(recv_pck->data + offset));
	offset += sizeof(int);
	file_info->chunk_number = *((int *)(recv_pck->data + offset));
	offset += sizeof(int);

	return 0;
}

int open_connection(struct transfer_node *dwnode) {
	if ((dwnode->socksd = tcp_socket()) < 0) {
		perror("downloader_func error - socket failed");
		return -1;
	}

	if (tcp_connect(dwnode->socksd, &dwnode->addrto) < 0) {
		perror("downloader_func error - tcp_connect failed");
		return -1;
	}

	return 0;
}

int create_file_part(const char *partname, const struct transfer_node *dwnode, int *miss_chunk_number, int *my_chunk_number) {
	char buf[256 + INFO_STR_LEN];
	int fdpart;
	int offset;

	if ((fdpart = open(partname, O_RDWR | O_CREAT, 0644)) < 0) {
		perror("create_file_part error - open failed");
		return -1;
	} else {
		*miss_chunk_number = dwnode->file_info.chunk_number;
		*my_chunk_number = 0;
		strcpy(buf, dwnode->file_info.filename);
		offset = strlen(dwnode->file_info.filename);
		buf[offset] = 0;
		offset ++;
		memcpy(buf + offset, dwnode->file_info.md5, MD5_DIGEST_LENGTH);
		offset += MD5_DIGEST_LENGTH;
		memcpy(buf + offset, (char *)&dwnode->file_info.file_size, sizeof(dwnode->file_info.file_size));
		offset += sizeof(dwnode->file_info.file_size);
		memcpy(buf + offset, (char *)&dwnode->file_info.chunk_number, sizeof(dwnode->file_info.chunk_number));
		offset += sizeof(dwnode->file_info.chunk_number);
		memcpy(buf + offset, (char *)my_chunk_number, sizeof(int));
		offset += sizeof(int);
		if (write(fdpart, buf, offset) < 0) {
			perror("create_file_part error - write failed");
			if (close(fdpart) < 0) {
				perror("create_file_part error - close failed");
			}
			return -1;
		}
	}

	return fdpart;

}

int load_file_part(int fdpart, const char *partname, struct transfer_node *dwnode, int *miss_chunk_number, int *my_chunk_number) {
	char buf[INFO_STR_LEN];
	int nread;

	if ((nread = readstr(fdpart, dwnode->file_info.filename, 255)) < 0) {
		perror("create_file_part error - readstr failed");
		return -1;
	}

	printf("File richiesto esistente, riprendo il download\n");
	printf("filename: %s\n", dwnode->file_info.filename);
	if ((nread = read(fdpart, buf, INFO_STR_LEN)) < 0) {
		perror("create_file_part error - read failed haha");
		if (close(fdpart) < 0) {
			perror("create_file_part error - close failed");
		}
		return -1;
	} else if (nread != INFO_STR_LEN) {
		fprintf(stderr, "create_file_part error - bad file format haha\n");
		if (close(fdpart) < 0) {
			perror("create_file_part error - close failed");
		}
		return -1;
	}

	if (memcmp(dwnode->file_info.md5, buf, MD5_DIGEST_LENGTH)) {
		fprintf(stderr, "create_file_part error - md5 doesn't match\n");
		if (close(fdpart) < 0) {
			perror("create_file_part error - close failed");
		}
		return -1;
	}

	if (dwnode->file_info.file_size != *((int *)(buf + MD5_DIGEST_LENGTH))) {
		fprintf(stderr, "create_file_part error - file size doesn't match\n");
		if (close(fdpart) < 0) {
			perror("create_file_part error - close failed");
		}
		return -1;
	}

	if (dwnode->file_info.chunk_number != *((int *)(buf + MD5_DIGEST_LENGTH + sizeof(int)))) {
		fprintf(stderr, "create_file_part error - chunk number doesn't match\n");
		if (close(fdpart) < 0) {
			perror("create_file_part error - close failed");
		}
		return -1;
	}

	*my_chunk_number = *((int *)(buf + MD5_DIGEST_LENGTH + sizeof(int) * 2));
	*miss_chunk_number = dwnode->file_info.chunk_number - *my_chunk_number;

	return 0;
}

int get_filepart_name(char *partname, const struct transfer_node *dwnode) {
	int fddownloads;
	char buf[263];
	unsigned char md5[MD5_DIGEST_LENGTH];
	int nread, rc;

	if ((rc = pthread_mutex_lock(&downloads_file_mutex)) != 0) {
		fprintf(stderr, "get_filepart_name error - can't acquire lock: %s\n", strerror(rc));
		return -1;
	}

	if ((fddownloads = open(DOWNLOADS_FILE, O_RDWR | O_CREAT, 0644)) < 0) {
		perror("get_filepart_name error - open failed");
		if ((rc = pthread_mutex_unlock(&downloads_file_mutex)) != 0) {
			fprintf(stderr, "get_filepart_name error - can't release lock: %s\n", strerror(rc));
		}
		return -1;
	}

	while ((nread = read(fddownloads, md5, MD5_DIGEST_LENGTH)) > 0) {
		if (nread != MD5_DIGEST_LENGTH) {
			fprintf(stderr, "get_filepart_name error - bad file format\n");
			if (close(fddownloads) < 0) {
				perror("get_filepart_name error - close failed");
			}
			if ((rc = pthread_mutex_unlock(&downloads_file_mutex)) != 0) {
				fprintf(stderr, "get_filepart_name error - can't release lock: %s\n", strerror(rc));
			}
			return -1;
		}

		if ((nread = readline(fddownloads, buf, 261)) < 0) {
			perror("get_filepart_name error - readline failed");
			if (close(fddownloads) < 0) {
				perror("get_filepart_name error - close failed");
			}
			if ((rc = pthread_mutex_unlock(&downloads_file_mutex)) != 0) {
				fprintf(stderr, "get_filepart_name error - can't release lock: %s\n", strerror(rc));
			}
			return -1;
		} else if (nread == 0) {
			fprintf(stderr, "get_filepart_name error - bad file format\n");
			if (close(fddownloads) < 0) {
				perror("get_filepart_name error - close failed");
			}
			if ((rc = pthread_mutex_unlock(&downloads_file_mutex)) != 0) {
				fprintf(stderr, "get_filepart_name error - can't release lock: %s\n", strerror(rc));
			}
			return -1;
		}

		buf[nread - 1] = 0;

		if (!memcmp(dwnode->file_info.md5, md5, MD5_DIGEST_LENGTH)) {
			strncpy(partname, buf, nread - 1);
			partname[nread - 1] = 0;
			break;
		}
	}

	if (nread == 0) {
		sprintf(partname, "%s/.%s.part\n", PART_DIR, dwnode->file_info.filename);
		if (write(fddownloads, dwnode->file_info.md5, MD5_DIGEST_LENGTH) < 0) {
			perror("get_filepart_name error - write failed");
			if (close(fddownloads) < 0) {
				perror("get_filepart_name error - close failed");
			}
			if ((rc = pthread_mutex_unlock(&downloads_file_mutex)) != 0) {
				fprintf(stderr, "get_filepart_name error - can't release lock: %s\n", strerror(rc));
			}
			return -1;
		}

		if (write(fddownloads, partname, strlen(partname)) < 0) {
			perror("get_filepart_name error - write failed");
			if (close(fddownloads) < 0) {
				perror("get_filepart_name error - close failed");
			}
			if ((rc = pthread_mutex_unlock(&downloads_file_mutex)) != 0) {
				fprintf(stderr, "get_filepart_name error - can't release lock: %s\n", strerror(rc));
			}
			return -1;
		}
		partname[strlen(partname) - 1] = 0;
	}

	if (close(fddownloads) < 0) {
		perror("get_filepart_name error - close failed");
		if ((rc = pthread_mutex_unlock(&downloads_file_mutex)) != 0) {
			fprintf(stderr, "get_filepart_name error - can't release lock: %s\n", strerror(rc));
		}
		return -1;
	}

	if ((rc = pthread_mutex_unlock(&downloads_file_mutex)) != 0) {
		fprintf(stderr, "get_filepart_name error - can't release lock: %s\n", strerror(rc));
		return -1;
	}

	return 0;
}

int open_file_part(char *partname, struct transfer_node *dwnode, int *miss_chunk_number, int *my_chunk_number) {
	int fdpart;

	if (get_filepart_name(partname, dwnode) < 0) {
		fprintf(stderr, "open_file_part error - get_filepart_name failed\n");
		return -1;
	}

	if ((fdpart = open(partname, O_RDWR)) < 0) {
		if (errno == ENOENT) {
			if ((fdpart = create_file_part(partname, dwnode, miss_chunk_number, my_chunk_number)) < 0) {
				fprintf(stderr, "open_file_part error - create_file_part failed\n");
				return -1;
			}
		} else {
			perror("create_file_part error - open failed");
			return -1;
		}
	} else {
		if (load_file_part(fdpart, partname, dwnode, miss_chunk_number, my_chunk_number) < 0) {
			fprintf(stderr, "open_file_part error - load_file_part failed\n");
			return -1;
		}
	}

	return fdpart;
}

int *get_missing_chunk(int fdpart, int chunk_number, int my_chunk_number, int miss_chunk_number) {
	int *my_chunk = NULL, *missing_chunk = NULL;
	int i, j, k, nread;

	if (my_chunk_number > 0) {
		my_chunk = (int *)malloc(my_chunk_number * sizeof(int));
	}
	missing_chunk = (int *)malloc(miss_chunk_number * sizeof(int));

	for (i = 0; i < my_chunk_number; i ++) {
		if ((nread = read(fdpart, (char *)&my_chunk[i], sizeof(int))) < 0) {
			perror("get_missing_chunk error - read failed");
			free(my_chunk);
			free(missing_chunk);
			return NULL;
		} else if (nread != sizeof(int)) {
			fprintf(stderr, "get_missing_chunk error - bad file format\n");
			free(my_chunk);
			free(missing_chunk);
			return NULL;
		}		
	}

	k = 0;
	for (i = 0; i < chunk_number; i ++) {
		for (j = 0; j < my_chunk_number; j ++) {
			if (i == my_chunk[j]) {
				break;
			}
		}
		if (j == my_chunk_number) {
			missing_chunk[k] = i;
			k ++;
		}
	}

	free(my_chunk);
	return missing_chunk;

}

int add_download(const struct sockaddr_in *addr, const unsigned char *md5) {
	int rc;

	if ((rc = pthread_mutex_lock(&download_list_mutex)) != 0) {
		fprintf(stderr, "add_download error - can't acquire lock: %s\n", strerror(rc));
		return -1;
	}

	insert_download(addr, md5);

	if ((rc = pthread_mutex_unlock(&download_list_mutex)) != 0) {
		fprintf(stderr, "add_download error - can't release lock: %s\n", strerror(rc));
		return -1;
	}

	return 0;
}

int downloader_init(int nthread, int th_pipe, pthread_mutex_t *mutex) {
	int i;

	if (dw_pool != NULL) {
		return 0;
	}

	dw_pool = (struct downloader_info *)malloc(sizeof(struct downloader_info) * nthread);
	for (i = 0; i < nthread; i ++) {
		dw_pool[i].th_pipe = th_pipe;
		dw_pool[i].pipe_mutex = mutex;
		if (downloader_run(&dw_pool[i]) < 0) {
			fprintf(stderr, "downloader_init error - downloader_run failed\n");
			return -1;
		}
	}

	return 0;
}

int downloader_run(struct downloader_info *dwinfo) {
	int rc;

	if ((rc = pthread_create(&dwinfo->thread, NULL, (void *)downloader_func, (void *)dwinfo)) != 0) {
		fprintf(stderr, "downloader_run error - can't create thread: %s\n", strerror(rc));
		return -1;
	}

	return 0;
}

