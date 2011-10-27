#ifndef _INET_UTIL_H
#define _INET_UTIL_H

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <netdb.h>

#include "ioutil.h"

#define ADDR_STR_LEN 6

int tcp_socket(void);

int udp_socket(void);

int set_addr_in(struct sockaddr_in *sad, const char *addr, short port);

void set_addr_any(struct sockaddr_in *sad, short port);

int tcp_connect(int socksd, const struct sockaddr_in *sad);

int inet_bind(int socksd, const struct sockaddr_in *sad);

int udp_send(int socksd, const struct sockaddr_in *sad, const char *buf, int len);

int udp_recvfrom(int socksd, struct sockaddr_in *sad, socklen_t *addr_len, char *buf, int maxlen);

int udp_recv(int socksd, char *buf, int maxlen);

void addrcpy(struct sockaddr_in *dest, const struct sockaddr_in *src);

int addrcmp(const struct sockaddr_in *sad1, const struct sockaddr_in *sad2);

struct sockaddr_in *str_to_addr(const char *str, int dim);

int addr2str(char *str, unsigned long addr, unsigned short port);

int str2addr(struct sockaddr_in *addr, const char *str);

int get_local_addr(int socksd, struct sockaddr_in *addr);

unsigned short get_local_port(int socksd);

int contains_addr(const char *str, unsigned int str_len, const struct sockaddr_in *addr);

int close_sock(int socksd);

#endif
