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
#include "downloader.h"
#include "uploader.h"
#include "state.h"



#define BS_PORT 5193

#define MAX_P_COUNT 1  //numero massimo di peer connessi a me stesso
#define MAX_REDIRECT_COUNT 0 //numero massimo di redirect

#define MAX_BS_ERROR 5

pthread_mutex_t pipe_mutex = PTHREAD_MUTEX_INITIALIZER;
int thread_pipe[2]; // usato per pipe	

long peer_rate;

int bserror;

int dw_listen_sock;
int transfer_initialized;

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

int init_transfer();

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

int add_sp_file(unsigned long ip, unsigned short port);

int send_share(int udp_sock, const struct sockaddr_in *addr);

int leave_handler(int udp_sock, const struct packet *recv_pck, const struct sockaddr_in *bs_addr, const struct sockaddr_in *addr);

int register_handler(int udp_sock, const struct sockaddr_in *addr, const struct packet *recv_pck);

void write_help();

void send_leave(int udp_sock);

void help();

void usage();

int keyboard_handler(int udp_sock);

void print_results_name(const struct addr_node *results, const char *name);

int whohas_request_handler(int socksd, int udp_sock, const struct packet *pck, const struct sockaddr_in *addr, int overlay);

int whohas_response_handler(int udp_sock, const struct packet *pck, const struct sockaddr_in *addr);

int whohas_handler_udp(int socksd, const struct packet *pck, const struct sockaddr_in *addr);

int whohas_request_handler_name(int udp_sock, const struct packet *pck, const struct sockaddr_in *addr);

int whohas_request_handler_md5(int udp_sock, const struct packet *pck, const struct sockaddr_in *addr);

int whohas_response_handler_md5(const struct packet *pck);

int whohas_response_handler_name(const struct packet *pck);

int upload_term_handler(int index);

int do_get(const char *str);

int abort_handler(int udp_sock, const struct sockaddr_in *addr, const struct packet *recv_pck);

int err_handler(int udp_sock, const struct sockaddr_in *addr, const struct sockaddr_in *bs_addr, const struct packet *recv_pck);

#endif

