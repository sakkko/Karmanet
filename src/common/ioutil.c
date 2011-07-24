#include "ioutil.h"

ssize_t writen(int fd, const void *buf, size_t n)
{
  size_t nleft;
  ssize_t nwritten;
  const char *ptr;

  ptr = buf;
  nleft = n;
  while (nleft > 0) {
    if ((nwritten = write(fd, ptr, nleft)) <= 0) {
       if ((nwritten < 0) && (errno == EINTR)) nwritten = 0; 	    
       else return(-1);	    /* errore */
    }
    nleft -= nwritten;
    ptr += nwritten;  
  }
  return(n-nleft);
}

int readline(int fd, void *vptr, int maxlen)
{
  int  n, rc;
  char c, *ptr;

  ptr = vptr;
  for (n = 1; n < maxlen; n++) {
    if ((rc = read(fd, &c, 1)) == 1) { 
      *ptr++ = c;
      if (c == '\n') break;
   } 
   else 
      if (rc == 0) {		/* read ha letto l'EOF */
 	 if (n == 1) return(0);	/* esce senza aver letto nulla */
 	 else break;
      } 
      else return(-1);		/* errore */
  }

  *ptr = 0;	/* per indicare la fine dell'input */
  return(n);	/* restituisce il numero di byte letti */
}

int lock(const char *p)
{
	int fd, retcode;
	while(1) {
		fd = open(p, O_RDWR|O_CREAT|O_EXCL, 0644);
		if((fd < 0) && (errno == EEXIST)) 
			sched_yield();
		else if (fd < 0) {
				perror("nome lock non valido\n");
				retcode = -1;
				break;
				}
		else {
			retcode = fd;
			break;
			}
		}
	return retcode;
}

int unlock(const char *p, int fd)
{
	int retcode = 0;
	
	retcode = close(fd);
	if (retcode < 0) {
		perror("file descriptor lock non valido\n");
		return retcode;
		}
	else {
		retcode = unlink(p);
		if (retcode < 0) {
			perror("bad unlink\n");
			return -1;
			}
		}
	return retcode;
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

void ltob(char *dest, long src) {
	char *it = (char *)&src;
	int i;

	for (i = 0; i < sizeof(long); i ++) {
		dest[i] = it[i];
	}
}

void stob(char *dest, short src) {
	char *it = (char *)&src;
	int i;

	for (i = 0; i < sizeof(short); i ++) {
		dest[i] = it[i];
	}
}

long btol(const char *str) {
	long *ret = (long *)str;
	return *ret;
}

short btos(const char *str) {
	short *ret = (short *)str;
	return *ret;
}

unsigned short get_rand() {
	srand(time(NULL));
	return (unsigned short)rand();
}

