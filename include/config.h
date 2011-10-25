#ifndef _CONFIG_H
#define _CONFIG_H

#include "ioutil.h"

#define CONFIG_FILE "../karma.conf"

#define UDP_PORT_FLAG "udp_port="
#define TCP_PORT_FLAG "tcp_port="
#define SHARE_FOLDER_FLAG "share_folder="
#define MAX_DOWNLOAD_FLAG "max_download="
#define MAX_UPLOAD_FLAG "max_upload="

struct config {
	unsigned short udp_port;
	unsigned short tcp_port;
	unsigned short max_download;
	unsigned short max_upload;
	char share_folder[256];
};

struct config conf;

int read_config(struct config *conf);

#endif
