#include "inetutil.h"


int tcp_socket(void) {
	return socket(AF_INET, SOCK_STREAM, 0);
}

int udp_socket(void) {
	return socket(AF_INET, SOCK_DGRAM, 0);
}

int set_addr_in(struct sockaddr_in *sad, const char *addr, short port) {
    memset((void *)sad, 0, sizeof(struct sockaddr_in));
	sad->sin_family = AF_INET;
	sad->sin_port = htons(port);
	if (inet_pton(AF_INET, addr, &(sad->sin_addr)) < 0) {
		return -1;
	}
	return 0;
}

void set_addr_any(struct sockaddr_in *sad, short port) {
	memset((void *)sad, 0, sizeof(struct sockaddr_in));
	sad->sin_family = AF_INET;
	sad->sin_addr.s_addr = htonl(INADDR_ANY);
	sad->sin_port = htons(port);
}

int tcp_connect(int socksd, const struct sockaddr_in *sad) {
	if (connect(socksd, (struct sockaddr *)sad, sizeof(struct sockaddr_in)) < 0) {
		return -1;
	}
	return 0;
}

int inet_bind(int socksd, const struct sockaddr_in *sad) {
	return bind(socksd, (struct sockaddr *)sad, sizeof(struct sockaddr_in));
}

int udp_send(int socksd, const struct sockaddr_in *sad, const char *buf, int len) {
	return sendto(socksd, buf, len, 0, (struct sockaddr *)sad, sizeof(struct sockaddr_in));
}

int udp_recvfrom(int socksd, struct sockaddr_in *sad, socklen_t *addr_len, char *buf, int maxlen) {
	return recvfrom(socksd, buf, maxlen, 0, (struct sockaddr *)sad, addr_len);
}

int udp_recv(int socksd, char *buf, int maxlen) {
	return recvfrom(socksd, buf, maxlen, 0, NULL, NULL);
}

int close_sock(int socksd) {
	if (socksd < 0) {
		return 0;
	} else {
		return close(socksd);
	}

}

void addrcpy(struct sockaddr_in *dest, const struct sockaddr_in *src) {
	dest->sin_family = src->sin_family;
	dest->sin_addr.s_addr = src->sin_addr.s_addr;
	dest->sin_port = src->sin_port;
}

int addrcmp(const struct sockaddr_in *sad1, const struct sockaddr_in *sad2) {
	return ((sad1->sin_addr.s_addr == sad2->sin_addr.s_addr) && (sad1->sin_port == sad2->sin_port));
}

struct sockaddr_in *str_to_addr(const char *str, int dim) {
	struct sockaddr_in *ret = (struct sockaddr_in *)malloc(dim * sizeof(struct sockaddr_in));
	int offset = sizeof(long) + sizeof(short);
	int i;
	
	for (i = 0; i < dim; i  ++) {
		str2addr(&ret[i], str + (i * offset));
	}
	
	return ret;
}

int addr2str(char *str, unsigned long addr, unsigned short port) {
	ltob(str, addr);
	stob(str + sizeof(addr), port);
	return 0;
}

int str2addr(struct sockaddr_in *addr, const char *str) {
	memset((void *)addr, 0, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = btol(str);
	addr->sin_port = btos(str + sizeof(long));
	return 0;
}

int get_local_addr(int socksd, struct sockaddr_in *addr) {
	unsigned int len = sizeof(struct sockaddr_in);
	return getsockname(socksd, (struct sockaddr *)addr, &len);
}

unsigned short get_local_port(int socksd) {
	struct sockaddr_in addr;
	if (get_local_addr(socksd, &addr) < 0) {
		return -1;
	}
	return addr.sin_port;
}

int contains_addr(const char *str, unsigned int str_len, const struct sockaddr_in *addr) {
	int offset = 0;

	while (offset < str_len) {
		if (!memcmp(str + offset, (char *)&addr->sin_addr.s_addr, sizeof(addr->sin_addr.s_addr))) {
			offset += sizeof(addr->sin_addr.s_addr);
			if (!memcmp(str + offset, (char *)&addr->sin_port, sizeof(addr->sin_port))) {
				return 1;
			}
			offset += sizeof(addr->sin_port);
		} else {
			offset += sizeof(addr->sin_addr.s_addr) + sizeof(addr->sin_port);
		}
	}

	return 0;
}


