#ifndef _SELECT_UTIL_H
#define _SELECT_UTIL_H

#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>

#define MAX_FD_SIZE 9

void fd_init();

int fd_add(int fd);

void fd_remove(int fd);

int fd_select();

int fd_ready(int fd);

#endif
