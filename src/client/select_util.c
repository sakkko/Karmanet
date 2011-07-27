#include "select_util.h"

int maxd;
int fds[MAX_FD_SIZE];
int fd_count;

fd_set allset;
fd_set rset;

void fd_init() {
	int i;

	maxd = -1;
	fd_count = 0;
	FD_ZERO(&allset);

	for (i = 0; i < MAX_FD_SIZE; i ++) {
		fds[i] = -1;
	}
}

int fd_add(int fd) {
	int i;

	if (fd_count >= MAX_FD_SIZE) {
		return -1;
	}

	FD_SET(fd, &allset);
	if (fd > maxd) {
		maxd = fd;
	}	
	for (i = 0; i < MAX_FD_SIZE; i ++) {
		if (fds[i] == -1) {
			fds[i] = fd;
			fd_count ++;
			break;
		}
	}

	return 0;

}

void fd_remove(int fd) {
	FD_CLR(fd, &allset);

	int i;

	for (i = 0; i < MAX_FD_SIZE; i ++) {
		if (fds[i] == fd) {
			fds[i] = -1;
			break;
		}
	}

	if (fd == maxd) {
		maxd = -1;
		for (i = 0; i < MAX_FD_SIZE; i ++) {
			if (fds[i] > maxd) {
				maxd = fds[i];
			}
		}
	}
}

int fd_select() {
	rset = allset;
	return select(maxd + 1, &rset, NULL, NULL, NULL);
}

int fd_ready(int fd) {
	return FD_ISSET(fd, &rset);
}

