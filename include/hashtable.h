#ifndef _HASHTABLE_H
#define _HASHTABLE_H

#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <malloc.h>
#include <unistd.h>
#include <fcntl.h>


#define HASH_PRIME_NUMBER_FILE 30011

#define HASH_PRIME_NUMBER_PEER 211

struct addr_node {
	unsigned long ip;
	unsigned short port;
	struct addr_node *prev;
	struct addr_node *next;
};

struct file_node {
	char name[255];
	struct addr_node *addr_list;
	struct file_node *prev;
	struct file_node *next;
};

struct file_info {
	struct file_node *file;
	struct addr_node *addr;
	struct file_info *prev;
	struct file_info *next;
};

struct ip_node {
	unsigned long ip;
	struct file_info *file_list;
	struct ip_node *prev;
	struct ip_node *next;
};

struct file_node *hash_file[HASH_PRIME_NUMBER_FILE];
struct ip_node *hash_ip[HASH_PRIME_NUMBER_PEER];

unsigned int filehash(const char* str);

int iphash(long ip);

struct file_node *create_new_file_node(const char *str, unsigned long ip, unsigned short port);

struct addr_node *create_new_addr_node(unsigned long ip, unsigned short port);

struct ip_node *create_new_ip_node(const struct file_node *file, const struct addr_node *addr);

struct file_info *create_new_file_info(const struct file_node *file, const struct addr_node *addr); 

int insert_file(const char *str, unsigned long ip, unsigned short port);

int remove_file(unsigned long ip, const char *str);

int remove_file_node(struct file_node *node);

int remove_addr_node(struct addr_node *addr, struct file_node *file);

int remove_ip_node(struct ip_node *ipnode);

int remove_file_info(struct file_info *fileinfo, struct ip_node *ipnode);

int insert_ip_node(unsigned long ip, const struct file_node *file, const struct addr_node *addr);

int remove_ip_from_file(const char *str, unsigned long ip, unsigned short port);

void print_file_table();

void print_ip_table();

int remove_all_file (unsigned long ip);

struct addr_node *get(char *file_name);

#endif
