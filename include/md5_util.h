#ifndef _MD5_UTIL_H
#define _MD5_UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <openssl/evp.h>
#include <openssl/md5.h>

int getMD5fd(int fd, unsigned char *md);

int getMD5fname(const char *name, unsigned char *md5);

void print_as_hex(const unsigned char *digest, int len);

void to_hex(char *strhex, const unsigned char *md5);

#endif
