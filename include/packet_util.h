#ifndef _PACKET_UTIL_H
#define _PACKET_UTIL_H

#include "command.h"
#include "ioutil.h"
#include "inetutil.h"

#define HEADER_SIZE CMD_STR_LEN + 7
#define MAX_PACKET_DATA_LEN 1024
#define MAX_PACKET_SIZE HEADER_SIZE + MAX_PACKET_DATA_LEN

#define DEFAULT_TTL 6

#define PACKET_FLAG_WHOHAS_NAME 1
#define PACKET_FLAG_WHOHAS_MD5 2
#define PACKET_FLAG_QUERY 8 //SET = QUERY UNSET = RESPONSE
#define PACKET_FLAG_NEXT_CHUNK 32
#define PACKET_FLAG_SUPERPEER 128

struct packet {
	char cmd[CMD_STR_LEN];
	char flag;
	unsigned short index;
	short TTL;
	unsigned short data_len;
	char data[MAX_PACKET_DATA_LEN];
};

unsigned short get_index();

void init_index();

void new_packet(struct packet *to_create, const char *cmd, unsigned short index, const char *data, unsigned short data_len, short ttl);

void new_join_packet(struct packet *pck, unsigned short index);

void new_join_packet_sp(struct packet *pck, unsigned short index, unsigned long rate, unsigned short dw_port);

void new_ack_packet(struct packet *pck, unsigned short index);

void new_err_packet(struct packet *pck, unsigned short index);

void new_ping_packet(struct packet *pck, unsigned short index);

void new_pong_packet(struct packet *pck, unsigned short index);

void new_promote_packet(struct packet *pck, unsigned short index);

void new_leave_packet(struct packet *pck, unsigned short index);

void new_redirect_packet(struct packet *pck, unsigned short index, struct sockaddr_in *child);

void new_register_packet(struct packet *pck, unsigned short index);

int get_pcklen(const struct packet *input_pck);

void pck_to_b(char *str, const struct packet *input);

void b_to_pck(const char *input, struct packet *output);

int recv_packet(int socksd, struct packet *pck);

int recvfrom_packet(int socksd, struct sockaddr_in *addr, struct packet *pck, int *len);

int send_packet(int socksd, const struct sockaddr_in *addr, const struct packet *pck);

void pckcpy(struct packet *dest, const struct packet *src);

int send_packet_tcp(int socksd, const struct packet *pck);
	
void add_near_to_packet(struct packet *pck, const char * data, int data_len);

void set_whohas_name_flag(struct packet *pck);

void set_whohas_md5_flag(struct packet *pck);

void unset_whohas_name_flag(struct packet *pck);

void unset_whohas_md5_flag(struct packet *pck);

void set_nextchunk_flag(struct packet *pck);

void unset_nextchunk_flag(struct packet *pck);

void set_superpeer_flag(struct packet *pck);

void unset_superpeer_flag(struct packet *pck);

int is_set_flag(const struct packet *pck, char flag);

void set_flag(struct packet *pck, char flag);

void unset_flag(struct packet *pck, char flag);

void set_query_flag(struct packet *pck);

void unset_query_flag(struct packet *pck);

void new_whs_query_packet(struct packet *pck, unsigned short index, const char *data, unsigned short data_len, short ttl);

void new_whs_query5_packet(struct packet *pck, unsigned short index, const char *data, unsigned short data_len, short ttl);

void new_whs_res_packet(struct packet *pck, unsigned short index, const char *data, unsigned short data_len, short ttl);

void new_whs_res5_packet(struct packet *pck, unsigned short index, const char *data, unsigned short data_len, short ttl);

int recv_packet_tcp(int socksd, struct packet *pck);

void new_get_packet(struct packet *pck, unsigned short index, const char *data, unsigned short data_len, short ttl);

#endif
