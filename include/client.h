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

#include "boot.h"
#include "superpeer.h"

#define MAXLINE		1024
#define DIM_P 		4
#define DIM_SP 		5

#define UDP_PORT 5193
#define BS_PORT 5193

#define MAX_REDIRECT_COUNT 10 //numero massimo di redirect


typedef void Sigfunc(int);

int fd[2]; // usato per pipe	

long peer_rate;

struct sockaddr_in my_sp_addr; //(indirizzo del sp a me associato)

void sig_chld_handler(int signo);

Sigfunc *signal(int signo, Sigfunc *func);


void set_rate();

int init(int *udp_sock, fd_set *fdset);

int udp_handler(int udp_sock, const struct sockaddr_in *bs_addr, fd_set *allset, struct sp_checker_info *spchinfo, struct peer_list_ch_info *plchinfo);

#endif

