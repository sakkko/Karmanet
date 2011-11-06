#ifndef _BOOTSTRAP_H
#define _BOOTSTRAP_H

#include <stdio.h>

#include "command.h"
#include "ioutil.h"
#include "inetutil.h"
#include "packet_util.h"
#include "sp_list.h"
#include "sp_list_checker.h"

#define ADDR_TOSEND 2

#define SERV_PORT 5193

int addr_to_send;

int sp_join(int sockfd, const struct sockaddr_in *addr, const struct packet *pck, struct sp_list_checker_info *splchinfo);

int sp_leave(int sockfd, const struct sockaddr_in *addr, const struct packet *pck, struct sp_list_checker_info *splchinfo);

int sp_register(int sockfd, const struct sockaddr_in *addr, const struct packet *pck, struct sp_list_checker_info *splchinfo);

int send_addr_list(int sockfd, const struct sockaddr_in *addr, const struct packet *pck);

int set_str_addrlist(char *str);

#endif
