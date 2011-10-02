#ifndef _CONFIG_H
#define _CONFIG_H

#include "ioutil.h"

#define CONFIG_FILE "../karma.conf"

#define UDP_PORT_FLAG "udp_port="
#define TCP_PORT_FLAG "tcp_port="
#define SHARE_FOLDER_FLAG "share_folder="

struct config {
	unsigned short udp_port;
	unsigned short tcp_port;
	char share_folder[256];
};

struct config conf;

int read_config(struct config *conf);

#endif
