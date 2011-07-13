#ifndef _BOOT_H
#define _BOOT_H

#include <stdio.h>

#include "command.h"
#include "ioutil.h"
#include "inetutil.h"
#include "packet_util.h"
#include "sp_list.h"

#define TIME_CHECK_FLAG 6
#define SERV_PORT 5193


int ping(const struct sockaddr_in *addr);

int leave(int sockfd, const struct sockaddr_in *addr, const struct packet *pck);

int send_addr_list(int sockfd, const struct sockaddr_in *addr, const struct packet *pck);

int join(int sockfd, const struct sockaddr_in *addr, const struct packet *pck);

int check_flag(void *unused);

int set_sp_flag(const struct sockaddr_in *addr, short flag_value);

int set_str_addrlist(char *str);

#endif
