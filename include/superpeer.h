#ifndef _SUPERPEER_H
#define _SUPERPEER_H

#include "ioutil.h"
#include "packet_util.h"
#include "peer_list_checker.h"
#include "request_list.h"
#include "retx.h"

#define BACKLOG 10
#define TCP_PORT 5193
#define MAX_TCP_SOCKET 6

#define MAX_P_COUNT 4  //numero massimo di peer connessi a me stesso

int tcp_listen;

int tcp_sock[MAX_TCP_SOCKET];

int free_sock;

int is_sp;

short curr_p_count; //per contare quanti peer sono connessi a me sp

short have_child; // per ricordare se ho fatto un promote

short curr_child_redirect; //per contare quanti redirect ho inviato su mio figlio

struct sockaddr_in child_addr; //ultimo promote inviato


int sp_init();

int join_overlay(fd_set *fdset, const struct sockaddr_in *sp_addr_list, int list_len);

int init_superpeer(fd_set *fdset, const struct sockaddr_in *sp_addr_list, int list_len);
 
int set_listen_socket(fd_set *fdset);

int join_peer(int udp_sock, const struct sockaddr_in *addr, const struct packet *pck, struct peer_list_ch_info *plchinfo);

#endif

