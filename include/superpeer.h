#ifndef _SUPERPEER_H
#define _SUPERPEER_H

#include "ioutil.h"
#include "packet_util.h"
#include "peer_list_checker.h"
#include "retx.h"
#include "select_util.h"
#include "hashtable.h"
#include "config.h"
#include "thread_util.h"

#include <errno.h>

#define BACKLOG 10
#define TCP_PORT 5193
#define MAX_TCP_SOCKET 6


char near_str[MAX_TCP_SOCKET * 6];

int tcp_listen;

int tcp_sock[MAX_TCP_SOCKET];

pthread_mutex_t tcp_lock[MAX_TCP_SOCKET];

int free_sock;

int is_sp;

short have_child; // per ricordare se ho fatto un promote

short curr_child_redirect; //per contare quanti redirect ho inviato su mio figlio

struct sockaddr_in child_addr; //ultimo promote inviato

struct sockaddr_in myaddr;

int sp_init();

int join_overlay(const struct sockaddr_in *sp_addr_list, int list_len);

int init_superpeer(const struct sockaddr_in *sp_addr_list, int list_len);
 
int set_listen_socket(unsigned short port);

int join_peer(const struct sockaddr_in *peer_addr, unsigned long peer_rate, struct peer_list_ch_info *plchinfo);

int promote_peer(int udp_sock, struct peer_list_ch_info *plchinfo);

int add_files(const struct sockaddr_in *peer_addr, const char *pck_data, int data_len);

int accept_conn(int tcp_listen);

int close_conn(int sock);

#endif

