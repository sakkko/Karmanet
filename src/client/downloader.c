#include "retx.h"

pthread_mutex_t download_list_mutex = PTHREAD_MUTEX_INITIALIZER;

void download_func(void *args) {
	struct downloader_info *dwinfo = (struct downloader_info *)args;
	int socksd;
	char cmdstr[CMD_STR_LEN + 1];
	struct timespec sleep_time;
	struct download_node *dwnode;
	int rc;
	char buf[MAX_LINE + 10];

	sleep_time.tv_sec = TIME_TO_SLEEP;
	sleep_time.tv_nsec = 0;

	while (1) {
		nanosleep(&sleep_time, NULL);
		if ((rc = pthread_mutex_lock(&download_mutex)) != 0) {
			fprintf(stderr, "retx_func error - can't acquire lock: %s\n", strerror(rc));
			pthread_exit((void *)-1);
		}

		dwnode = pop_download();
		
		if ((rc = pthread_mutex_unlock(&retx_mutex)) != 0) {
			fprintf(stderr, "retx_func error - can't release lock: %s\n", strerror(rc));
			pthread_exit((void *)-1);
		}
		if (dwnode == NULL) {
			continue;
		}
		
		if ((socksd = tcp_socket()) < 0) {
			perror("downloader_func error - socket failed");
//			write_err(dwinfo->
			continue;
		}
		
		if (tcp_connect(socksd, &dwinfo->addrto) < 0) {
			perror("downloader_func error - tcp_connect failed");
			continue;
		}
		
		//MANDARE PACCHETTO GET CON MD5
		struct packet get_pck;
		new_get_packet(&get_pck,0, dwnode->md5,MD5_DIGEST_LENGTH,0);
		
		if(send_packet_tcp(socksd, &get_pck) < 0 ){
			fprintf(stderr,"downloader_func error - send_packet_tcp failed");	
		}
		
		if ((n = read(socksd, buf, MAX_LINE + 9)) < 0) {
			perror("bad read");
			return 1;
		}

		printf("letti %d bytes\n", n);

		if (strncmp(buf, "INF", 3)) {
			printf("unknown operation\n");
			return 1;
		}

		int i;
		tmpch = strstr(buf + 4, "\n");
		if (tmpch == NULL) {
			perror("bad strstr\n");
			return 1;
		}
		
		i = (int)(tmpch - buf - 4);
		strncpy(name, buf + 4, i);
		name[i] = 0;
		i += 5;
		printf("name: %s\n", name);


		int j;

		memcpy(md5, buf + i, 16);

		i += 16;
		print_as_hex(md5, 16);
		printf("\n");

		int nchunk;
		int flen;

		flen = *((int *)(buf + i));
		printf("file size: %d\n", flen);

		i += sizeof(int);

		nchunk = *((int *)(buf + i));
		printf("chunk: %d\n", nchunk);


		int fdpart;
		int *my_chunk = NULL;
		int *missing_chunk = NULL;
		sprintf(partname, "./part/%s.part", name);

		printf("part file: %s\n", partname);


		if ((fdpart = open(partname, O_RDWR)) < 0) {
			if (errno == ENOENT) {
				printf("File non esistente, lo creo\n");

				if ((fdpart = open(partname, O_RDWR | O_CREAT, 0644)) < 0) {
					perror("bad open");
					return 1;
				} else {
					write(fdpart, buf + 4, n - 4);
				}

			} else {
				perror("bad open");
				return 1;
			}
		} else {
			lseek(fdpart, strlen(name) + 1, SEEK_SET);
			if (read(fdpart, md52, 16) != 16) {
				fprintf(stderr, "bad file format\n");
				return 1;
			}

			print_as_hex(md52, 16);
			printf("\n");

			if (memcmp(md5, md52, 16)) {
				fprintf(stderr, "md5 doesn't match\n");
				return 1;
			}
		
			int nchunk2;
			read(fdpart, (char *)&myflen, sizeof(int));
			read(fdpart, (char *)&nchunk2, sizeof(int));
			read(fdpart, (char *)&nmychunk, sizeof(int));

			if (nchunk != nchunk2) {
				fprintf(stderr, "Chunk number doesn't match\n");
				return 1;
			}

			if (flen != myflen) {
				fprintf(stderr, "File size doesn't match\n");
				return 1;
			}

			printf("My-CHUNK: %d\n", nmychunk);

			if (nmychunk == nchunk) {
				printf("File già completo\n");
				return 1;
			}


			nmisschunk = nchunk - nmychunk;
			my_chunk = (int *)malloc(nmychunk * sizeof(int));
			missing_chunk = (int *)malloc(nmisschunk * sizeof(int));

			for (i = 0; i < nmychunk; i ++) {
				read(fdpart, (char *)&my_chunk[i], sizeof(int));
				printf("%d\n", my_chunk[i]);
			}

			int k = 0;
			for (i = 0; i < nchunk; i ++) {
				for (j = 0; j < nmychunk; j ++) {
					if (i == my_chunk[j]) {
						break;
					}
				}
				if (j == nmychunk) {
					missing_chunk[k] = i;
					printf("missing_chunk[%d]: %d\n", k, missing_chunk[k]);
					k ++;
				}
			}
		}

		sprintf(tmpname, "./download/%s", name);

		printf("file name: %s\n", tmpname);

		if ((fd = open(tmpname, O_WRONLY | O_CREAT, 0644)) < 0) {
			perror("bad open");
			return 1;
		}

		//int mychunK = 0;

		if (missing_chunk == NULL) {
			for (j = 0; j < nchunk; j ++) {
			
				if (j == nchunk - 1) {
					printf("CIAO\n");
				}
				strncpy(buf, "GET ", 4);
				memcpy(buf + 4, (char *)&j, sizeof(int));

				write(socksd, buf, sizeof(int) + 4);

				while ((n = read(socksd, buf, MAX_LINE + 9)) > 0) {
					if (strncmp("DAT", buf, 3)) {
						printf("bad command\n");
						return 1;
					}
				
					if (*((int *)(buf + 4)) != j) {
						printf("bad chunk\n");
						return 1;
					}

					write(fd, buf + 9, n - 9);

					if (buf[8] == 0) {
					
						lseek(fdpart, strlen(name) + 17 + sizeof(int) + sizeof(int), SEEK_SET);
						nmychunk ++;
						tmpch = (char *)&nmychunk;
						write(fdpart, tmpch, sizeof(int));

						lseek(fdpart, 0, SEEK_END);
						tmpch = (char *)&j;
						write(fdpart, tmpch, sizeof(int));

						break;
					}
			
				}
			}

			if (close(socksd) < 0) {
				perror("bad close");
				return 1;
			}

			if (lseek(fd, 0, SEEK_END) == flen && nmychunk == nchunk) {
				if (unlink(partname) < 0) {
					perror("bad unlink");
					return 1;
				}
			}
		} else {
			for (j = 0; j < nmisschunk; j ++) {
				strncpy(buf, "GET ", 4);
				memcpy(buf + 4, (char *)&missing_chunk[j], sizeof(int));
				write(socksd, buf, sizeof(int) + 4);
				lseek(fd, missing_chunk[j] * CHUNK_SIZE, SEEK_SET);
				while ((n = read(socksd, buf, MAX_LINE + 9)) > 0) {
					buf[n] = 0;
					if (strncmp("DAT", buf, 3)) {
						printf("bad command\n");
						return 1;
					}
					if (*((int *)(buf + 4)) != missing_chunk[j]) {
						printf("bad chunk\n");
						return 1;
					}
					write(fd, buf + 9, n - 9);
					if (buf[8] == 0) {
						lseek(fdpart, strlen(name) + 17 + sizeof(int) + sizeof(int), SEEK_SET);
						nmychunk ++;
						tmpch = (char *)&nmychunk;
						write(fdpart, tmpch, sizeof(int));
						lseek(fdpart, 0, SEEK_END);
						tmpch = (char *)&missing_chunk[j];
						write(fdpart, tmpch, sizeof(int));
						break;
					}			
				}
			}

			if (close(socksd) < 0) {
				perror("bad close");
				return 1;
			}

			if (lseek(fd, 0, SEEK_END) == flen && nmychunk == nchunk) {
				if (unlink(partname) < 0) {
					perror("bad unlink");
					return 1;
				}
			}
		}


	}
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
	dw_pool = (struct downloader_node *)malloc(sizeof(struct downloader_node *) * nthread);

	for (i = 0; i < nthread; i ++) {
		dw_pool[i].th_pipe = pipe;
		dw_pool[i].pipe_mutex = murtex;
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

