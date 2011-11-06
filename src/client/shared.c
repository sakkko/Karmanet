#include "shared.h"

int selall(const struct dirent *dirname) {
	if (dirname->d_name[0] == '.') {
		return 0;
	} else if (dirname->d_type == DT_REG || dirname->d_type == DT_DIR) {
		return 1;
	} else {
		return 0;
	}
}

int selfile(const struct dirent *dirname) {
	if (dirname->d_name[0] == '.') {
		return 0;
	} else if (dirname->d_type == DT_REG){
		return 1;
	} else {
		return 0;
	}
}

int seldir(const struct dirent *dirname) {
	if (dirname->d_name[0] == '.') {
		return 0;
	} else if (dirname->d_type == DT_DIR){
		return 1;
	} else {
		return 0;
	}

}

/*
 * Scrive una direcory nel file share.
 */
int write_dir(int fd, int modtime, const char *dir) {
	char tmp[MAXLINE];

	*tmp = '#';
	memcpy(tmp + 1, (char *)&modtime, sizeof(modtime));
	sprintf(tmp + sizeof(modtime) + 1, "%s\n", dir);
	
	if (write(fd, tmp, sizeof(modtime) + strlen(dir) + 2) < 0) {
		perror("write_dir error - bad write");
		return -1;
	}

	return 0;
}

/*
 * Scrive un file al file share.
 */
int write_file(int fd, int modtime, const unsigned char *md5, const char *filename) {
	char tmp[276];
	*tmp = '%';
	memcpy(tmp + 1, (char *)&modtime, sizeof(modtime));
	memcpy(tmp + sizeof(modtime) + 1, md5, MD5_DIGEST_LENGTH);
	sprintf(tmp + MD5_DIGEST_LENGTH + sizeof(modtime) + 1, "%s\n", filename);
	if (write(fd, tmp, MD5_DIGEST_LENGTH + sizeof(modtime) + strlen(filename) + 2) < 0) {
		perror("write_file error - write failed");
		return -1;
	}

	return 0;
}

/*
 * Scrive un file al file diff.
 */
int write_diff(int fd, char flag, const unsigned char *md5, const char *filename) {
	char tmp2[272];

	*tmp2 = flag;
	memcpy(tmp2 + 1, md5, MD5_DIGEST_LENGTH);
	sprintf(tmp2 + MD5_DIGEST_LENGTH + 1, "%s\n", filename);
	if (write(fd, tmp2, MD5_DIGEST_LENGTH + strlen(filename) + 2) < 0) {
		perror("write_diff error - bad write");
		return -1;
	}

	return 0;
}

/*
 * Aggiunge un file, scrive il file share e il file diff.
 */
int add_shared_file(int fd_share, int fd_diff, const struct dirent *file, const char *share_dir) {
	char filepath[MAXLINE];
	unsigned char md5[MD5_DIGEST_LENGTH];
	struct stat st;

	sprintf(filepath, "%s/%s", share_dir, file->d_name);
	if (lstat(filepath, &st) < 0) {
		perror("add_shared_file error - lstat failed");
		return -1;
	}
	if (getMD5fname(filepath, md5) < 0) {
		fprintf(stderr, "add_shared_file error - getMD5fname failed\n");
		return -1;
	}

	if (write_file(fd_share, st.st_mtime, md5, file->d_name) < 0) {
		fprintf(stderr, "add_shared_file error - write_file failed\n");
		return -1;
	}
	if (write_diff(fd_diff, '+', md5, file->d_name) < 0) {
		fprintf(stderr, "add_shared_file error - write_diff failed\n");
		return -1;
	}

	return 0;

}

/*
 * Aggiunge tutti i file contenuti nella lista.
 */
int add_shared_files(int fd_share, int fd_diff, struct dirent **flist, int start_indx, int nfile, const char *dir) {
	int i;

	for (i = start_indx; i < nfile; i ++) {
		if (add_shared_file(fd_share, fd_diff, flist[i], dir) < 0) {
			fprintf(stderr, "add_share_files error - add file failed\n");
			return -1;
		}
		free(flist[i]);
	}

	return i;
}

/*
 * Aggiunge una directory.
 */
int add_dir(int fdnew, int fddiff, const char *share_dir) {
	int nfile;
	struct stat st;
	struct dirent **flist;


	if ((nfile = scandir(share_dir, &flist, selfile, alphasort)) < 0) {
		perror("add_dir error - scandir failed");
		return -1;
	}
	if (lstat(share_dir, &st) < 0) {
		perror("add_dir error - lstat failed");
		return -1;
	}

	if (write_dir(fdnew, st.st_mtime, share_dir) < 0) {
		fprintf(stderr, "add_dir error - write_dir failed\n");
		return -1;
	}

	if (add_shared_files(fdnew, fddiff, flist, 0, nfile, share_dir) < 0) {
		fprintf(stderr, "add_dir error - add_files failed\n");
		return -1;
	}

	return 0;
}

/*
 * Modifica una file nel file share e diff.
 */
int update_file(int fdnew, int fddiff, time_t modtime, const char *fpath, const char *fname, const unsigned char *old_md5) {
	char tmp3[33];
	unsigned char md5[MD5_DIGEST_LENGTH];

	printf("Calcolo nuovo md5 per %s\n", fname);
	getMD5fname(fpath, md5); 
	to_hex(tmp3, md5);
	tmp3[32] = 0;
	printf("Nuovo md5: %s\n", tmp3);

	if (write_diff(fddiff, '-', old_md5, fname) < 0) {
		fprintf(stderr, "update_file error - write_diff failed\n");
		return -1;
	}
	if (write_diff(fddiff, '+', md5, fname) < 0) {
		fprintf(stderr, "update_file error - write_diff failed\n");
		return -1;
	}
	if (write_file(fdnew, modtime, md5, fname) < 0) {
		fprintf(stderr, "update_file error - write_file failed\n");
		return -1;
	}

	return 0;

}

/*
 * Rimuove una directory: scrive file share e diff.
 */
int remove_dir(int fd_share, int fd_diff) {
	int nread;
	char buf[MAXLINE];
	char fname[255];

	while ((nread = read(fd_share, buf, MD5_DIGEST_LENGTH + sizeof(int) + 1)) > 0) {
		if (*buf == '#') {
			lseek(fd_share, -1 * nread, SEEK_CUR);
			break;
		} else if (*buf != '%' || nread != MD5_DIGEST_LENGTH + sizeof(int) + 1) {
			fprintf(stderr, "remove_dir error - bad file format\n");
			return -1;
		}

		if ((nread = readline(fd_share, fname, 255)) < 0) {
			perror("remove_dir error - bad read");
			return -1;
		}

		fname[nread - 1] = 0;
		if (write_diff(fd_diff, '-', (const unsigned char *)(buf + sizeof(int) + 1), fname) < 0) {
			fprintf(stderr, "remove_dir error - write_diff failed\n");
			return -1;
		}
	}

	return 0;


}

/*
 * Rimuove tutti i file presenti nel file share.
 */
int remove_all_files(int fd_share, int fd_diff) {
	int nread;
	char buf[MAXLINE];
	char fname[255];

	while ((nread = read(fd_share, buf, sizeof(int) + 1)) > 0) {
		if (nread != sizeof(int) + 1) {
			fprintf(stderr, "remove_all_files error - bad file format\n");
			return -1;
		}

		if (*buf == '#') {
			if ((nread = readline(fd_share, fname, 255)) < 0) {
				perror("remove_all_files error - bad read");
				return -1;
			}
			continue;
		} else if (*buf != '%') {
			fprintf(stderr, "remove_all_files error - bad file format\n");
			return -1;
		}

		if (read(fd_share, buf, MD5_DIGEST_LENGTH) < 0) {
			perror("remove_all_files error - bad read");
			return -1;
		}
		if ((nread = readline(fd_share, fname, 255)) < 0) {
			perror("remove_all_files error - bad read");
			return -1;
		}

		//printf("%s", fname);
		fname[nread - 1] = 0;
		if (write_diff(fd_diff, '-', (const unsigned char *)buf, fname) < 0) {
			fprintf(stderr, "remove_all_files error - write_diff failed\n");
			return -1;
		}
	}

	return 0;

}

/*
 * Modifica una directory nel file share.
 */
int update_dir(int fd_share, int fdnew, int fddiff, const char *share_dir) {
	int nread, nfile;
	struct dirent **flist;
	struct stat st;
	char buf[MAXLINE];
	char tmp[MAXLINE];
	char fname[255];
	time_t modtime, mtime;
	int i = 0, m;

	if ((nfile = scandir(share_dir, &flist, selfile, alphasort)) < 0) {
		perror("update_dir error - dirent failed");
		return -1;
	}

	while (i < nfile) {				
		if ((nread = read(fd_share, buf, MD5_DIGEST_LENGTH + sizeof(modtime) + 1)) < 0) {
			perror("update_dir error - read failed");
			return -1;
		}

		if (nread == 0) {
			if ((i = add_shared_files(fdnew, fddiff, flist, i, nfile, share_dir)) < 0) {
				fprintf(stderr, "update_dir error - add_files failed\n");
				return -1;
			}
		} else {
			if (*buf == '#') {
				lseek(fd_share, -1 * nread, SEEK_CUR);
				break;
			} 
			if (nread != MD5_DIGEST_LENGTH + sizeof(modtime) + 1 || *buf != '%') {
				fprintf(stderr, "update_dir error - bad file format\n");
				return -1;
			}

			if ((nread = readline(fd_share, fname, 256)) < 0) {
				perror("update_dir error - read failed");
				return -1;
			}

			fname[nread - 1] = 0;

			if ((m = strcmp(fname, flist[i]->d_name)) == 0) {
				sprintf(tmp, "%s/%s", share_dir, fname);
				if (lstat(tmp, &st) < 0) {
					perror("update_dir error - lstat failed");
					return -1;
				}

				modtime = st.st_mtime;
				mtime = *((time_t *)(buf + 1));
				if (mtime == modtime) {
					sprintf(buf + MD5_DIGEST_LENGTH + sizeof(modtime) + 1, "%s\n", fname);
					write(fdnew, buf, MD5_DIGEST_LENGTH + sizeof(modtime) + strlen(fname) + 2);
				} else {
					if (update_file(fdnew, fddiff, modtime, tmp, fname, (const unsigned char *)(buf + sizeof(modtime) + 1)) < 0) {
						fprintf(stderr, "update_dir error - update_file failed\n");
						return -1;
					}
				}
				free(flist[i]);
				i ++;
			} else if (m < 0) {
				if (write_diff(fddiff, '-', (const unsigned char *)(buf + sizeof(modtime) + 1), fname) < 0) {
					fprintf(stderr, "update_dir error - write_diff failed\n");
					return -1;
				}
			} else {
				if (add_shared_file(fdnew, fddiff, flist[i], share_dir) < 0) {
					fprintf(stderr, "update_dir error - add_shared_file failed\n");
					return -1;
				}
				lseek(fd_share, -1 * (nread + MD5_DIGEST_LENGTH + sizeof(modtime) + 1), SEEK_CUR);
				free(flist[i]);
				i ++;

			}
		}
	}

	if (i < nfile) {
		if (add_shared_files(fdnew, fddiff, flist, i, nfile, share_dir) < 0) {
			fprintf(stderr, "update_dir error - add_shared_files failed\n");
			return -1;
		}
	}
	if (remove_dir(fd_share, fddiff) < 0) {
		fprintf(stderr, "update_dir error - remove_dir failed\n");
		return -1;
	}

	return 0;
}

/*
 * Copia una directory nel file share.
 */
int copy_dir(int fd_share, int fdnew) {
	int nread;
	char buf[MAXLINE];

	while ((nread = read(fd_share, buf, MD5_DIGEST_LENGTH + sizeof(time_t) + 1)) > 0) {
		if (nread != MD5_DIGEST_LENGTH + sizeof(time_t) + 1) {
			fprintf(stderr, "copy_dir error - bad file format\n");
			return -1;
		}
		if (*buf == '#') {
			lseek(fd_share, -1 * nread, SEEK_CUR);
			break;
		} else {
			if ((nread = readline(fd_share, buf + MD5_DIGEST_LENGTH + sizeof(time_t) + 1, MAXLINE)) < 0) {
				perror("copy_dir error - bad read");
				return -1;
			}
			if (write(fdnew, buf, nread + MD5_DIGEST_LENGTH + sizeof(time_t) + 1) < 0) {
				perror("copy_dir error - write failed\n");
				return -1;
			}
		}
	}

	return 0;

}

/*
 * Aggiunge i file e le directory al file share e al file diff.
 */
int diff(int fd_share, int fdnew, int fddiff, const char *share_dir, int flag) {
	int ndir, nread, n, m;
	struct dirent **dlist;
	char tmp2[MAXLINE];
	char buf[MAXLINE];
	struct stat st;
	time_t modtime;
	int mtime;

	if ((ndir = scandir(share_dir, &dlist, seldir, alphasort)) < 0) {
		perror("diff error - dirent failed");
		return -1;
	}	

	if ((nread = read(fd_share, buf, sizeof(modtime) + 1)) < 0) {
		perror("diff error - read failed");
		return -1;
	}

	if (nread == 0) {
		if (add_dir(fdnew, fddiff, share_dir) < 0) {
			fprintf(stderr, "diff error - add_dir failed\n");
			return -1;
		}
	} else {
		if (nread != sizeof(modtime) + 1 || *buf != '#') {
			fprintf(stderr, "diff error - bad file format\n");
			return -1;
		}

		mtime = *((int *)(buf + 1));

		if ((nread = readline(fd_share, tmp2, MAXLINE)) < 0) {
			perror("diff error - readline failed");
			return -1;
		}

		tmp2[nread - 1] = 0;
	//	printf("dir name: %s\n", tmp2);

		if ((m = strcmp(tmp2, share_dir)) == 0) {
			if (lstat(share_dir, &st) < 0) {
				perror("diff error - lstat failed");
				return -1;
			}

			modtime = st.st_mtime;
			write_dir(fdnew, modtime, share_dir);
			if (mtime == modtime) {
				if (copy_dir(fd_share, fdnew) < 0) {
					fprintf(stderr, "diff error - copy_dir failed\n");
					return -1;
				}
			} else {
				if (update_dir(fd_share, fdnew, fddiff, share_dir) < 0) {
					fprintf(stderr, "diff error - update_dir failed\n");
					return -1;
				}
			}
		} else if (m < 0) {
			if (remove_dir(fd_share, fddiff) < 0) {
				fprintf(stderr, "diff error - remove_dir failed\n");
				return -1;
			}
			return diff(fd_share, fdnew, fddiff, share_dir, 1);
		} else {
			if (add_dir(fdnew, fddiff, share_dir) < 0) {
				fprintf(stderr, "diff error - add_dir failed\n");
				return -1;
			}
			lseek(fd_share, -1 * (sizeof(modtime) + nread + 1), SEEK_CUR);
		}
				
	}

	for (n = 0; n < ndir; n ++) {
		sprintf(tmp2, "%s/%s", share_dir, dlist[n]->d_name);
		diff(fd_share, fdnew, fddiff, tmp2, 1);
		free(dlist[n]);

	}

	if (flag == 0) {
		if (remove_all_files(fd_share, fddiff) < 0) {
			fprintf(stderr, "diff error - remove_all_files failed\n");
			return -1;
		}
	}

	return 0;
}

/*
 * Crea il file share e il file diff.
 */
int create_diff(const char *share_dir) {
	int fdshare, fdnew, fddiff;

	if ((fdshare = open(FILE_SHARE, O_RDONLY)) < 0) {
		if (errno != ENOENT) {
			perror("create_diff error - open failed");
			return -1;
		}
		if ((fdshare = open(FILE_SHARE, O_RDWR | O_CREAT, 0644)) < 0) {
			perror("create_diff error - open failed");
			return -1;
		}
	}

	if ((fdnew = open(FILE_TMP, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
		perror("create_diff error - open failed");
		return -1;
	}

	if ((fddiff = open(FILE_DIFF, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
		perror("create_diff error - open failed");
		return -1;
	}

	if (diff(fdshare, fdnew, fddiff, share_dir, 0) < 0) {
		fprintf(stderr, "create_diff error - diff failed\n");
		return -1;
	}

	if (close(fdshare) < 0) {
		perror("create_diff error - close failed");
		return -1;
	}
	
	if (close(fdnew) < 0) {
		perror("create_diff error - close failed");
		return -1;
	}
	
	if (close(fddiff) < 0) {
		perror("create_diff error - close failed");
		return -1;
	}

	if (unlink(FILE_SHARE) < 0) {
		perror("create_diff error - unlink failed");
		return -1;
	}

	if (rename(FILE_TMP, FILE_SHARE) < 0) {
		perror("create_diff error - rename failed");
		return -1;
	}

	return 0;
}

/*
 * Cerca nel file share e setta il filepath del file avente l'md5 
 * passato come parametro.
 * Ritorna -1 in caso di errore, 0 se il file è stato trovato, 
 * 1 se il file non è stato trovato.
 */
int get_filepath(char *filepath, char *filename, const unsigned char *md5) {
	int fd;
	char buf[MAXLINE];
	char dirname[MAXLINE];
	char md5_str[MD5_DIGEST_LENGTH * 2 + 1];
	int n;

	if ((fd = open(FILE_SHARE, O_RDONLY)) < 0) {
		perror("get_filepath error - open failed");
		return -1;
	}

	while ((n = read(fd, buf, sizeof(time_t) + 1)) > 0) {
		if (n != sizeof(time_t) + 1 || (*buf != '#' && *buf != '%')) {
			fprintf(stderr, "get_filepath error - bad file format\n");
			if (close(fd) < 0) {
				perror("get_filepath error - close failed");
			}
			return -1;
		}

		if (*buf == '#') {
			if ((n = readline(fd, dirname, MAXLINE)) < 0) {
				perror("get_filepath error - readline failed");
				if (close(fd) < 0) {
					perror("get_filepath error - close failed");
				}
				return -1;
			} else if (n == 0) {
				fprintf(stderr, "get_filepath error - bad file format\n");
				if (close(fd) < 0) {
					perror("get_filepath error - close failed");
				}
				return -1;
			}

			dirname[n - 1] = 0;
		} else {
			if ((n = read(fd, buf, MD5_DIGEST_LENGTH)) < 0) {
				perror("get_filepath error - read failed");
				if (close(fd) < 0) {
					perror("get_filepath error - close failed");
				}
				return -1;
			} else if (n != MD5_DIGEST_LENGTH) {
				fprintf(stderr, "get_filepath error - bad file format\n");
				if (close(fd) < 0) {
					perror("get_filepath error - close failed");
				}
				return -1;
			}

			to_hex(md5_str, (const unsigned char *)buf);
			md5_str[MD5_DIGEST_LENGTH * 2] = 0;

			if ((n = readline(fd, filename, 255)) < 0) {
				perror("get_filepath error - readline failed");
				if (close(fd) < 0) {
					perror("get_filepath error - close failed");
				}
				return -1;
			} else if (n == 0) {
				fprintf(stderr, "get_filepath error - bad file format\n");
				if (close(fd) < 0) {
					perror("get_filepath error - close failed");
				}
				return -1;
			}

			filename[n - 1] = 0;
			if (!memcmp(buf, md5, MD5_DIGEST_LENGTH)) {
				sprintf(filepath, "%s/%s", dirname, filename);
				printf("%s/%s\n", dirname, filename);
				printf("%s\n", filepath);
				if (close(fd) < 0) {
					perror("get_filepath error - close failed");
				}
				return 0;	
			}
		}

	}


	if (close(fd) < 0) {
		perror("get_filepath error - close failed");
		return -1;
	}

	return 1;
}

