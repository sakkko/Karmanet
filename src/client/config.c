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
	//	printf("%s\n", str);
		if (*str == '#') {
			continue;
		}
		if (!strncmp(str, UDP_PORT_FLAG, strlen(UDP_PORT_FLAG))) {
		//	printf("UDP_FLAG\n");
			strcpy(str2, str + strlen(UDP_PORT_FLAG));
			conf->udp_port = atoi(str2);
		} else if (!strncmp(str, TCP_PORT_FLAG, strlen(TCP_PORT_FLAG))) {
		//	printf("TCP_FLAG\n");
			strcpy(str2, str + strlen(TCP_PORT_FLAG));
			conf->tcp_port = atoi(str2);
		} else if (!strncmp(str, SHARE_FOLDER_FLAG, strlen(SHARE_FOLDER_FLAG))) {
		//	printf("SHARE_FLAG\n");
			strcpy(conf->share_folder, str + strlen(SHARE_FOLDER_FLAG));
		}
	//	printf("%s\n", str2);
	}

	printf("UDP_PORT = %u\nTCP_PORT = %u\nSHARE_FOLDER = %s\n", conf->udp_port, conf->tcp_port, conf->share_folder);

	if (close(fd) < 0) {
		perror("read_config error - close failed");
		return -1;
	}

	return 0;

}

