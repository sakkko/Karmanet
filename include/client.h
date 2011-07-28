#ifndef _CLIENT_H
#define _CLIENT_H

#include <unistd.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <signal.h>
#include <sys/wait.h>

#include "command.h"
#include "hashtable.h"
#include "inetutil.h"
#include "ioutil.h"
#include "peerrate.h"
#include "peer_list.h"
#include "pinger.h"
#include "request_list.h"
#include "packet_util.h"
#include "retx.h"
#include "peer_list_checker.h"
#include "sp_checker.h"
#include "select_util.h"

#include "boot.h"
#include "superpeer.h"

#define MAXLINE		1024
#define DIM_P 		4
#define DIM_SP 		5

#define ST_JOINBS_SENT 1
#define ST_JOINSP_SENT 2
#define ST_PROMOTE_SENT 3
#define ST_PROMOTE_RECV 4
#define ST_REGISTER_SENT 5
#define ST_ACTIVE 6
#define ST_LEAVE_SENT 7

#define UDP_PORT 5193
#define BS_PORT 5193

#define MAX_P_COUNT 1  //numero massimo di peer connessi a me stesso
#define MAX_REDIRECT_COUNT 10 //numero massimo di redirect

int udp_sock;

int fd[2]; // usato per pipe	

long peer_rate;

int state;

struct sp_checker_info *spchinfo;
struct peer_list_ch_info *plchinfo;
struct pinger_info pinfo;

struct sockaddr_in *addr_list;
int curr_addr_index;

struct sockaddr_in *sp_addr;

void set_rate();

int init();

int udp_handler(int udp_sock, const struct sockaddr_in *bs_addr);

int join_handler(int udp_sock, const struct sockaddr_in *addr, const struct packet *pck);

int promote_handler(int udp_sock, const struct sockaddr_in *bs_addr);

int end_process();

int run_threads();

int stop_threads();

#endif

