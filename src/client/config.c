#include "config.h"

int read_config(struct config *conf) {
	int fd;
	char str[255], str2[255];
	int n;

	if ((fd = open(CONFIG_FILE, O_RDONLY)) < 0) {
		perror("read_config error - open failed");
		return -1;
	}

	while ((n = readline(fd, str, 255)) > 0) {
		str[n - 1] = 0;
		if (*str == '#') {
			continue;
		}
		if (!strncmp(str, UDP_PORT_FLAG, strlen(UDP_PORT_FLAG))) {
			strcpy(str2, str + strlen(UDP_PORT_FLAG));
			conf->udp_port = atoi(str2);
		} else if (!strncmp(str, TCP_PORT_FLAG, strlen(TCP_PORT_FLAG))) {
			strcpy(str2, str + strlen(TCP_PORT_FLAG));
			conf->tcp_port = atoi(str2);
		} else if (!strncmp(str, SHARE_FOLDER_FLAG, strlen(SHARE_FOLDER_FLAG))) {
			strcpy(conf->share_folder, str + strlen(SHARE_FOLDER_FLAG));
		} else if (!strncmp(str, MAX_DOWNLOAD_FLAG, strlen(MAX_DOWNLOAD_FLAG))) {
			strcpy(str2, str + strlen(MAX_DOWNLOAD_FLAG));
			conf->max_download = atoi(str2);
		} else if (!strncmp(str, MAX_UPLOAD_FLAG, strlen(MAX_UPLOAD_FLAG))) {
			strcpy(str2, str + strlen(MAX_UPLOAD_FLAG));
			conf->max_upload = atoi(str2);
		}
	}

	printf("UDP_PORT = %u\nTCP_PORT = %u\nSHARE_FOLDER = %s\nMAX_DOWNLOAD = %d\nMAX_UPLOAD = %d\n", conf->udp_port, conf->tcp_port, conf->share_folder, conf->max_download, conf->max_upload);

	if (close(fd) < 0) {
		perror("read_config error - close failed");
		return -1;
	}

	return 0;

}

