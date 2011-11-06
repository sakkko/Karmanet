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

#define MAXLINE	1024

int maxd;

void add_fd(int fd, fd_set *fdset);

ssize_t writen(int fd, const void *buf, size_t n);

int readn(int fd, void *buf, size_t n);

int readline(int fd, void *vptr, int maxlen);

void ltob(char *dest, long src);

void stob(char *dest, short src);

unsigned long btol(const char *str);

unsigned short btos(const char *str);

unsigned short get_rand();

void get_dirpath(char *destpath, const char *fullpath);

int get_file_size(int fd);

int readstr(int fd, void *vptr, int maxlen);

#endif

