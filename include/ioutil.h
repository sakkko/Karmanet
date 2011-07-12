#ifndef _IOUTIL_H
#define _IOUTIL_H

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sched.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>

#define LOCK_FILE "list.lck"   //lock per la lista dei super peer usato dal bootstrap

#define LOCK_MY_SP  "my_sp.lck"  //lock per il super peer usato dal singolo peer

#define LOCK_MY_P  "my_p.lck"  //lock per la lista dei peer usato dal singolo super peer


ssize_t writen(int fd, const void *buf, size_t n);

int readline(int fd, void *vptr, int maxlen);

int lock(const char *p);

int unlock(const char *p, int fd);

int addr2str(char *str, unsigned long addr, unsigned short port);

int str2addr(struct sockaddr_in *addr, const char *str);

void ltob(char *dest, long src);

void stob(char *dest, short src);

long btol(const char *str);

short btos(const char *str);

short get_rand();

#endif

