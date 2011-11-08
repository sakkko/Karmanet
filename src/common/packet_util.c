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

void new_packet(struct packet *to_create, const char *cmd, unsigned short index, const char *data, unsigned short data_len, short ttl) {
	strcpy(to_create->cmd, cmd);
	to_create->index = index;
	to_create->flag = 0;
	to_create->data_len = data_len;
	to_create->TTL = ttl;
	if (data != NULL) {
		memcpy(to_create->data, data, data_len);
	}
}

void new_whs_query_packet(struct packet *pck, unsigned short index, const char *data, unsigned short data_len, short ttl) {
	new_packet(pck, CMD_WHOHAS, index, data, data_len, ttl);
	set_whohas_name_flag(pck);
	set_query_flag(pck);	
}

void new_whs_query5_packet(struct packet *pck, unsigned short index, const char *data, unsigned short data_len, short ttl) {
	new_packet(pck, CMD_WHOHAS, index, data, data_len, ttl);
	set_whohas_md5_flag(pck);
	set_query_flag(pck);	
}

void new_whs_res_packet(struct packet *pck, unsigned short index, const char *data, unsigned short data_len, short ttl) {
	new_packet(pck, CMD_WHOHAS, index, data, data_len, ttl);
	set_whohas_name_flag(pck);
}

void new_whs_res5_packet(struct packet *pck, unsigned short index, const char *data, unsigned short data_len, short ttl) {
	new_packet(pck, CMD_WHOHAS, index, data, data_len, ttl);
	set_whohas_md5_flag(pck);
}

void new_get_packet(struct packet *pck, unsigned short index, const char *data, unsigned short data_len, short ttl) {
	new_packet(pck, CMD_GET, index, data, data_len, ttl);
	set_query_flag(pck);
}

void new_join_packet(struct packet *pck, unsigned short index) {
	new_packet(pck, CMD_JOIN, index, NULL, 0, 1);
}


void new_join_packet_sp(struct packet *pck, unsigned short index, unsigned long rate, unsigned short dw_port) {
	char str[sizeof(long) + sizeof(short)];
	ltob(str, rate);
	stob(str + sizeof(long), dw_port);
	new_packet(pck, CMD_JOIN, index, str, sizeof(long) + sizeof(short), 1);
}

void new_ack_packet(struct packet *pck, unsigned short index) {
	new_packet(pck, CMD_ACK, index, NULL, 0, 1);
}

void new_err_packet(struct packet *pck, unsigned short index) {
	new_packet(pck, CMD_ERR, index, NULL, 0, 1);
}

void new_ping_packet(struct packet *pck, unsigned short index) {
	new_packet(pck, CMD_PING, index, NULL, 0, 1);
}

void new_pong_packet(struct packet *pck, unsigned short index) {
	new_packet(pck, CMD_PONG, index, NULL, 0, 1);
}

void new_promote_packet(struct packet *pck, unsigned short index) {
	new_packet(pck, CMD_PROMOTE, index, NULL, 0, 1);
}

void new_leave_packet(struct packet *pck, unsigned short index) {
	new_packet(pck, CMD_LEAVE, index, NULL, 0, 1);
}

void new_register_packet(struct packet *pck, unsigned short index) {
	new_packet(pck, CMD_REGISTER, index, NULL, 0, 1);
}

void new_redirect_packet(struct packet *pck, unsigned short index, struct sockaddr_in *child) {
	char str[ADDR_STR_LEN];
	addr2str(str, child->sin_addr.s_addr, child->sin_port);
	new_packet(pck, CMD_REDIRECT, index, str, ADDR_STR_LEN, 1);
}

int get_pcklen(const struct packet *input_pck) {
	return CMD_STR_LEN + sizeof(input_pck->index) + sizeof(input_pck->flag) + sizeof(input_pck->TTL) + sizeof(input_pck->data_len) + input_pck->data_len;
}
 
void pck_to_b(char *str, const struct packet *input) {
	int offset = CMD_STR_LEN;
	
	memcpy(str, input->cmd, CMD_STR_LEN);
	str[offset] = input->flag;
	offset ++;
	stob(str + offset, input->index);
	offset += sizeof(input->index);
	stob(str + offset, input->TTL);
	offset += sizeof(input->TTL);
	stob(str + offset, input->data_len);
	offset += sizeof(input->data_len);
	memcpy(str + offset, input->data, input->data_len);
}

void b_to_header(const char *input, struct packet *output) {
	int offset = CMD_STR_LEN;

	memcpy(output->cmd, input, CMD_STR_LEN);
	output->flag = input[offset];
	offset ++;
	output->index = btos(input + offset);
	offset += sizeof(output->index);
	output->TTL = btos(input + offset);
	offset += sizeof(output->TTL);
	output->data_len = btos(input + offset);
	offset += sizeof(output->data_len);
}

void b_to_pck(const char *input, struct packet *output) {
	int offset = CMD_STR_LEN;
	
	memcpy(output->cmd, input, CMD_STR_LEN);
	output->flag = input[offset];
	offset ++;
	output->index = btos(input + offset);
	offset += sizeof(output->index);
	output->TTL = btos(input + offset);
	offset += sizeof(output->TTL);
	output->data_len = btos(input + offset);
	offset += sizeof(output->data_len);
	memcpy(output->data, input + offset, output->data_len);
	output->data[output->data_len] = 0;

}

int recvfrom_packet(int socksd, struct sockaddr_in *addr, struct packet *pck, int *len) {
	int ret;
	char buf[MAX_PACKET_SIZE];
	
	if ((ret = udp_recvfrom(socksd, addr, (socklen_t *)len, buf, MAX_PACKET_SIZE)) < 0) {
		return -1;
	} else if (ret == 0) {
		return 0;
	}
	
	b_to_pck(buf, pck);
	return ret;
}

int recv_packet(int socksd, struct packet *pck) {
	return recvfrom_packet(socksd, NULL, pck, NULL);
}	

int recv_packet_tcp(int socksd, struct packet *pck) {
	int ret;
	char buf[HEADER_SIZE];

	if ((ret = readn(socksd, buf, HEADER_SIZE)) < 0) {
		perror("recv_packet_tcp error - read failed");
		return -1;
	} else if (ret > 0) {
		return 0;
	}

	b_to_header(buf, pck);

	if (pck->data_len > 0) {
		if ((ret = readn(socksd, pck->data, pck->data_len)) < 0) {
			perror("recv_packet_tcp error - read failed");
			return -1;
		} else if (ret > 0) {
			return 0;
		}
	}

	return pck->data_len + HEADER_SIZE;
}
	
int send_packet(int socksd, const struct sockaddr_in *addr, const struct packet *pck) {
	int ret;
	char buf[MAX_PACKET_SIZE];
	
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
	if ((ret = writen(socksd, buf, get_pcklen(pck))) < 0) {
		return -1;
	}
	
	return ret;
}
	
void pckcpy(struct packet *dest, const struct packet *src) {
	strncpy(dest->cmd, src->cmd, CMD_STR_LEN);
	dest->index = src->index;
	dest->flag = src->flag;
	dest->TTL = src->TTL;
	dest->data_len = src->data_len;
	if (dest->data_len > 0) {
		memcpy(dest->data, src->data, dest->data_len);
	}
}
	
void add_near_to_packet(struct packet *pck, const char *data, int data_len ){
	memcpy(pck->data,data,data_len);
	pck->data_len = data_len;
}

void set_whohas_name_flag(struct packet *pck) {
	set_flag(pck, PACKET_FLAG_WHOHAS_NAME);
}

void set_whohas_md5_flag(struct packet *pck) {
	set_flag(pck, PACKET_FLAG_WHOHAS_MD5);
}

void set_query_flag(struct packet *pck) {
	set_flag(pck, PACKET_FLAG_QUERY);
}

void set_nextchunk_flag(struct packet *pck) {
	set_flag(pck, PACKET_FLAG_NEXT_CHUNK);
}

void unset_whohas_name_flag(struct packet *pck) {
	unset_flag(pck, PACKET_FLAG_WHOHAS_NAME);
}

void unset_whohas_md5_flag(struct packet *pck) {
	unset_flag(pck, PACKET_FLAG_WHOHAS_MD5);
}

void unset_query_flag(struct packet *pck) {
	unset_flag(pck, PACKET_FLAG_QUERY);
}

void unset_nextchunk_flag(struct packet *pck) {
	unset_flag(pck, PACKET_FLAG_NEXT_CHUNK);
}

void set_superpeer_flag(struct packet *pck) {
	set_flag(pck, PACKET_FLAG_SUPERPEER);
}

void unset_superpeer_flag(struct packet *pck) {
	unset_flag(pck, PACKET_FLAG_SUPERPEER);
}

int is_set_flag(const struct packet *pck, char flag) {
	return pck->flag & flag;
}

void set_flag(struct packet *pck, char flag) {
	pck->flag |= flag;
}

void unset_flag(struct packet *pck, char flag) {
	pck->flag &= ~flag;
}

