#ifndef _BOOT_H
#define _BOOT_H

#include "packet_util.h"
#include "pinger.h"
#include "retx.h"
#include "sp_checker.h"
#include "superpeer.h"

struct sockaddr_in *get_sp_list(int udp_sock, const struct sockaddr_in *bs_addr, int *len, int *error);

int connect_to_sp(int udp_sock, struct sockaddr_in *sp_addr, const struct sockaddr_in *addr_list, int addr_list_len, unsigned long peer_rate);

int call_sp(int udp_sock, const struct sockaddr_in *addr_to_call, unsigned long peer_rate);

int start_process(int udp_sock, fd_set *fdset, struct sockaddr_in *sp_addr, const struct sockaddr_in *bs_addr, unsigned long peer_rate);

int end_process(fd_set *allset, int *fd, struct sp_checker_info *spchinfo, struct peer_list_ch_info *plchinfo, struct pinger_info *pinfo);

#endif

