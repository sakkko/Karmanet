#include "packet_util.h"

unsigned short start_index;

unsigned short get_index() {
	unsigned short ret = start_index;
	start_index ++;
	return ret;
}

void init_index() {
	start_index = get_rand();
}

int new_packet(struct packet *to_create, char *cmd, unsigned short index, char *data, unsigned short data_len, short ttl) {
	strcpy(to_create->cmd, cmd);
	to_create->index = index;
	to_create->data_len = data_len;
	to_create->TTL = ttl;
	if (data != NULL) {
		memcpy(to_create->data, data, data_len);
	}

	return 0;
}

int new_join_packet(struct packet *pck, unsigned short index) {
	return new_packet(pck, CMD_JOIN, index, NULL, 0, 1);
}


int new_join_packet_rate(struct packet *pck, unsigned short index , long rate) {
	char str[sizeof(long)];
	ltob(str, rate);
	return new_packet(pck, CMD_JOIN, index, str, 4, 1);
}

int new_ack_packet(struct packet *pck, unsigned short index) {
	return new_packet(pck, CMD_ACK, index, NULL, 0, 1);
}

int new_err_packet(struct packet *pck, unsigned short index) {
	return new_packet(pck, CMD_ERR, index, NULL, 0, 1);
}

int new_ping_packet(struct packet *pck, unsigned short index) {
	return new_packet(pck, CMD_PING, index, NULL, 0, 1);
}

int new_pong_packet(struct packet *pck, unsigned short index) {
	return new_packet(pck, CMD_PONG, index, NULL, 0, 1);
}

int new_promote_packet(struct packet *pck, unsigned short index) {
	return new_packet(pck, CMD_PROMOTE, index, NULL, 0, 1);
}

int new_leave_packet(struct packet *pck, unsigned short index) {
	return new_packet(pck, CMD_LEAVE, index, NULL, 0, 1);
}

int new_register_packet(struct packet *pck, unsigned short index) {
	return new_packet(pck, CMD_REGISTER, index, NULL, 0, 1);
}

int new_redirect_packet(struct packet *pck, unsigned short index, struct sockaddr_in *child) {
	char str[ADDR_STR_LEN];
	addr2str(str, child->sin_addr.s_addr, child->sin_port);
	return new_packet(pck, CMD_REDIRECT, index, str, ADDR_STR_LEN, 1);
}

int get_pcklen(const struct packet *input_pkg) {
	return CMD_STR_LEN + sizeof(short) + sizeof(short) + sizeof(short) + input_pkg->data_len;
}
 
void pck_to_b(char *str, const struct packet *input) {
	int offset = CMD_STR_LEN;
	
	memcpy(str, input->cmd, CMD_STR_LEN);
	stob(str + offset, input->index);
	offset += sizeof(input->index);
	stob(str + offset, input->TTL);
	offset += sizeof(input->TTL);
	stob(str + offset, input->data_len);
	offset += sizeof(input->TTL);
	memcpy(str + offset, input->data, input->data_len);
}

void b_to_pck(const char *input, struct packet *output) {
	int offset = CMD_STR_LEN;
	
	memcpy(output->cmd, input, CMD_STR_LEN);
	output->index = btos(input + offset);
	offset += sizeof(output->index);
	output->TTL = btos(input + offset);
	offset += sizeof(output->TTL);
	output->data_len = btos(input + offset);
	offset += sizeof(output->data_len);
	memcpy(output->data, input + offset, output->data_len);
}

int recvfrom_packet(int socksd, struct sockaddr_in *addr, struct packet *pck, int *len) {
	int ret;
	
	if ((ret = udp_recvfrom(socksd, addr, (socklen_t *)len, buf, MAX_PACKET_SIZE)) < 0) {
		return -1;
	}
	
	b_to_pck(buf, pck);
	return ret;
}

int recv_packet(int socksd, struct packet *pck) {
	return recvfrom_packet(socksd, NULL, pck, NULL);
}	
	
int send_packet(int socksd, const struct sockaddr_in *addr, const struct packet *pck) {
	int ret;
	
	pck_to_b(buf, pck);	
	if ((ret = udp_send(socksd, addr, buf, get_pcklen(pck))) < 0) {
		return -1;
	}
	
	return ret;
}

int send_packet_tcp(int socksd, const struct packet *pck) {
	int ret;
	char buf[MAX_PACKET_SIZE];
	pck_to_b(buf, pck);	
	if ((ret = write(socksd, buf, get_pcklen(pck))) < 0) {
		return -1;
	}
	
	return ret;
}


	
void pckcpy(struct packet *dest, const struct packet *src) {
	strncpy(dest->cmd, src->cmd, CMD_STR_LEN);
	dest->index = src->index;
	dest->TTL = src->TTL;
	dest->data_len = src->data_len;
	if (dest->data_len > 0) {
		memcpy(dest->data, src->data, dest->data_len);
	}
}
	
void add_near_to_packet(struct packet *pck,const char * data, int data_len ){
	
	memcpy(pck->data,data,data_len);
	pck->data_len = data_len;

}


