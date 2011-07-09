#ifndef _CLIENT_H
#define _CLIENT_H

#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>
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
#include "packet_util.h"
#include "retx.h"



#define BACKLOG		10
#define MAXLINE		1024
#define DIM_P 		4
#define DIM_SP 		5
//ricordiamoci di cambiarlo
#define TIME_CHECK_FLAG 10

#define TCP_PORT 5193
#define UDP_PORT 5193
#define BS_PORT 5193

#define MAX_P_COUNT 20  //numero massimo di peer connessi a me stesso
#define MAX_REDIRECT_COUNT 10 //numero massimo di redirect

#define MAX_TCP_SOCKET 6

typedef void Sigfunc(int);

int maxd;

int free_sock;

int tcp_sock[MAX_TCP_SOCKET];

int udp_sock;

int tcp_listen;

short have_child; // per ricordare se ho fatto un promote

short curr_child_redirect; //per contare quanti redirect ho inviato su mio figlio

short curr_p_count; //per contare quanti peer sono connessi a me sp

short is_sp;

long peer_rate;

short start_index;

struct sockaddr_in child; //ultimo promote inviato

struct sockaddr_in bs_addr; //indirizzo del boot

struct sockaddr_in my_sp_addr; //(indirizzo del sp a me associato)

void sig_chld_handler(int signo);

Sigfunc *signal(int signo, Sigfunc *func);

int super_peer(void *unused);

struct sockaddr_in *str_to_addr(const char *str, int dim);

short get_index();

void init_index();

void set_rate();

void add_fd(int fd, fd_set *fdset);

#endif

