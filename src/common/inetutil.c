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

void addrcpy(struct sockaddr_in *dest, const struct sockaddr_in *src) {
	dest->sin_family = src->sin_family;
	dest->sin_addr.s_addr = src->sin_addr.s_addr;
	dest->sin_port = src->sin_port;
}

int addrcmp(const struct sockaddr_in *sad1, const struct sockaddr_in *sad2) {
	return ((sad1->sin_addr.s_addr == sad2->sin_addr.s_addr) && (sad1->sin_port == sad2->sin_port));
}


