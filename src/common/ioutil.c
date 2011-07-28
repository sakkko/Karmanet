#include "ioutil.h"

void add_fd(int fd, fd_set *fdset) {
	FD_SET(fd, fdset);
	if (fd > maxd) {
		maxd = fd;
	}	
}

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

unsigned long btol(const char *str) {
	unsigned long *ret = (unsigned long *)str;
	return *ret;
}

unsigned short btos(const char *str) {
	unsigned short *ret = (unsigned short *)str;
	return *ret;
}

unsigned short get_rand() {
	srand(time(NULL));
	return (unsigned short)rand();
}

