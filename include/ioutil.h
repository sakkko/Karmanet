#ifndef _IOUTIL_H
#define _IOUTIL_H

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>

int maxd;

void add_fd(int fd, fd_set *fdset);

ssize_t writen(int fd, const void *buf, size_t n);

int readline(int fd, void *vptr, int maxlen);

void ltob(char *dest, long src);

void stob(char *dest, short src);

unsigned long btol(const char *str);

unsigned short btos(const char *str);

unsigned short get_rand();

#endif

