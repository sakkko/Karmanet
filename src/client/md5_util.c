#include "md5_util.h"

void to_hex(char *strhex, const unsigned char *md5) {
	char str[3];
	int i;

	for (i = 0; i < 16; i ++) {
		sprintf(str, "%02x", md5[i]);

	//	printf("%s", str);
		strhex[i * 2] = str[0];
		strhex[i * 2 + 1] = str[1];
	}
//	printf("\n");
}

int getMD5fd(int fd, unsigned char *md) {
	MD5_CTX md5ctx;
	char buf[8192];
	unsigned long n;
	if (MD5_Init(&md5ctx) == 0) {
		fprintf(stderr, "MD5_Init failed\n");
		return -1;
	}

	while ((n = read(fd, buf, 8192)) > 0) {
		if (MD5_Update(&md5ctx, buf, n) == 0) {
			fprintf(stderr, "MD5_Update failed\n");
			return -1;
		}
	}

	if (MD5_Final(md, &md5ctx) == 0) {
		fprintf(stderr, "MD5_Final failed\n");
		return -1;
	}

	return 0;

}

int getMD5fname(const char *name, unsigned char *md5) {
	int fd;

	if ((fd = open(name, O_RDONLY)) < 0) {
		perror("getMD5name error - open failed");
		return -1;
	}

	if (getMD5fd(fd, md5) < 0) {
		fprintf(stderr, "getMD5file failed\n");
		close(fd);
		return -1;
	}

	if (close(fd) < 0) {
		perror("bad close");
		return -1;
	}

	return 0;
}

void print_as_hex(const unsigned char *digest, int len) {
	int i;
	for(i = 0; i < len; i++){
		printf("%02x", digest[i]);
	}
	printf("\n");
}

void get_from_hex(const char *strhex, unsigned char *digest, int len) {
	int i;
	unsigned int tmp;

	for (i = 0; i < len; i ++) {
		sscanf(strhex +  i * 2, "%02x", &tmp);
		digest[i] = (unsigned char)tmp;
	}
}


