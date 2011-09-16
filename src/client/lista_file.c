#include "lista_file.h"

int selfile(const struct dirent *dirname) {
	if (dirname->d_name[0] == '.') {
		return 0;
	} else if (dirname->d_type == DT_REG) {
		return 1;
	} else {
		return 0;
	}
}

int seldir(const struct dirent *dirname) {
	if (dirname->d_name[0] == '.') {
		return 0;
	} else if (dirname->d_type == DT_DIR) {
		return 1;
	} else {
		return 0;
	}
	
}

int write_share_file(int fd_share, const char *dirname) {
	struct dirent **file_list;
	struct dirent **dir_list;
	int nfile, ndir, i;
	char str[255];
	
	if ((nfile = scandir(dirname, &file_list, selfile, NULL)) < 0) {
		perror("write_share_file error - scandir failed for file");
		return -1;
	}

	if ((ndir = scandir(dirname, &dir_list, seldir, NULL)) < 0) {
		perror("write_share_file error - scandir failed for directory");
		return -1;
	}

	if (nfile > 0) {
		snprintf(str, 255, "#%s\n", dirname);
		if (write(fd_share, str, strlen(str)) < 0) {
			perror("write_share_file error - write failed");
			return -1;
		}

		for (i = 0; i < nfile; i ++) {
			snprintf(str, 255, "%s\n", file_list[i]->d_name);
			if (write(fd_share, str, strlen(str)) < 0) {
				perror("write_share_file error - write failed");
				return -1;
			}
			free(file_list[i]);
		}
	}

	for (i = 0; i < ndir; i ++) {
		snprintf(str, 255, "%s/%s", dirname, dir_list[i]->d_name);
		if (write_share_file(fd_share, str) < 0) {
			free(dir_list[i]);
			return -1;
		}
		free(dir_list[i]);
	}
	
	return 0;
}

void share_file(char *dir_path, char *file_log){
	int fd = open (file_log, O_WRONLY | O_TRUNC | O_CREAT, 0644);
	write_share_file(fd, dir_path);
	close(fd);
}

