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
#include "near_list.h"

#include <errno.h>

#define MAX_P_COUNT 100  //numero massimo di peer connessi a me stesso
#define MAX_REDIRECT_COUNT 50 //numero massimo di redirect
#define BACKLOG 10
#define MAX_TCP_SOCKET 6
#define MAX_JOIN_OV_TRY 5


char *near_str;
int near_str_len;

int tcp_listen;

int nsock;

int is_sp;

int max_tcp_sock;

//int curr_redirect_count;//

short have_child; // per ricordare se ho fatto un promote

short child_miss_pong; //conto il numero di pong persi

short curr_child_redirect; //per contare quanti redirect ho inviato su mio figlio

pthread_mutex_t CHILD_INFO_LOCK;

short curr_child_p_count;

struct sockaddr_in child_addr; //ultimo promote inviato

struct sockaddr_in father_addr;

struct sockaddr_in myaddr;

int join_ov_try;


int join_overlay(const struct sockaddr_in *sp_addr_list, int list_len);

int init_superpeer(const struct sockaddr_in *sp_addr_list, int list_len);
 
int set_listen_socket(unsigned short port);

int join_peer(const struct sockaddr_in *peer_addr, unsigned long peer_rate, unsigned short dw_port, struct peer_list_ch_info *plchinfo);

int promote_peer(int udp_sock, struct peer_list_ch_info *plchinfo);

int add_files(const struct sockaddr_in *peer_addr, const char *pck_data, int data_len);

int accept_conn(int tcp_listen);

int close_conn(int sock);

void set_near();

int flood_overlay(const struct packet *pck, int socksd);

#endif

