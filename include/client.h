#ifndef _CLIENT_H
#define _CLIENT_H

#include <unistd.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <signal.h>
#include <sys/wait.h>


#include "near_list.h"
#include "command.h"
#include "config.h"
#include "hashtable.h"
#include "inetutil.h"
#include "ioutil.h"
#include "peerrate.h"
#include "peer_list.h"
#include "pinger.h"
#include "packet_util.h"
#include "retx.h"
#include "peer_list_checker.h"
#include "sp_checker.h"
#include "select_util.h"
#include "superpeer.h"
#include "shared.h"


#define ST_JOINBS_SENT 1
#define ST_JOINSP_SENT 2
#define ST_PROMOTE_SENT 3
#define ST_PROMOTE_RECV 4
#define ST_REGISTER_SENT 5
#define ST_ACTIVE 6
#define ST_LEAVE_SENT 7
#define ST_FILELIST_SENT 8

#define UDP_PORT 5193
#define BS_PORT 5193

#define MAX_P_COUNT 2  //numero massimo di peer connessi a me stesso
#define MAX_REDIRECT_COUNT 1 //numero massimo di redirect

#define MAX_BS_ERROR 5

pthread_mutex_t pipe_mutex = PTHREAD_MUTEX_INITIALIZER;
int thread_pipe[2]; // usato per pipe	

long peer_rate;

int state;

int bserror;

struct sp_checker_info *sp_checker;
struct peer_list_ch_info *peer_list_checker;
struct pinger_info pinger;
struct retx_info retx;

struct sockaddr_in *addr_list;
int addr_list_len;
int curr_addr_index;
int curr_redirect_count;

void set_rate();

int init();

int udp_handler(int udp_sock, const struct sockaddr_in *bs_addr);

int join_handler(int udp_sock, const struct sockaddr_in *addr, const struct packet *pck);

int promote_handler(int udp_sock, const struct sockaddr_in *recv_addr, const struct sockaddr_in *bs_addr, const struct packet *recv_pck);

int list_handler(int udp_sock, const struct sockaddr_in *bs_addr, const struct packet *recv_pck);

int redirect_handler(int udp_sock, const struct packet *recv_pck);

int ping_handler(int udp_sock, const struct sockaddr_in *addr, const struct packet *rcv_pck);

int ack_handler(int udp_sock, const struct sockaddr_in *addr, const struct sockaddr_in *bs_addr, const struct packet *recv_pck);

int error_handler(int udp_sock, const char *errstr, const struct sockaddr_in *bs_addr);

int update_file_list_handler(int udp_sock, const struct sockaddr_in *addr, const struct packet *recv_pck);

void pong_handler();

int end_process(int reset);

int run_threads(int udp_sock, const struct sockaddr_in *bs_addr, const struct sockaddr_in *sp_addr);

int stop_threads(int reset);

int send_share_file(int udp_sock, const struct sockaddr_in *addr);

int add_sp_file(const struct sockaddr_in *addr);

int send_share(int udp_sock, const struct sockaddr_in *addr);

int leave_handler(int udp_sock, const struct packet *recv_pck, const struct sockaddr_in *bs_addr, const struct sockaddr_in *addr);

void write_help();

void send_leave(int udp_sock);

void help();

void usage();

int keyboard_handler(int udp_sock);

void print_results_name(const struct addr_node *results, const char *name);

int whohas_request_handler(int socksd, int udp_sock, const struct packet *pck, const struct sockaddr_in *addr, int overlay);

int whohas_response_handler(int udp_sock, const struct packet *pck, const struct sockaddr_in *addr);

int whohas_handler_udp(int socksd, const struct packet *pck, const struct sockaddr_in *addr);

#endif

