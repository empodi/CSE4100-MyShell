/* $begin myshell.h */
#ifndef __MYSHELL_H__
#define __MYSHELL_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <signal.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* Misc constants */
#define	MAXLINE	 8192  /* Max text line length */
#define MAXBUF   8192  /* Max I/O buffer size */
#define LISTENQ  1024  /* Second argument to listen() */
#define MAXARGS 128
#define MAXFD 1024

/* External variables */
extern int h_errno;    /* Defined by BIND for DNS errors */ 
extern char **environ; /* Defined by libc */

typedef void handler_t(int);

/* Function prototypes */
void eval(char* cmdline, int *fd);
int parseline(char *buf, char **argv);
int builtin_command(char **argv); 

int CD(char** argv);

/* Sio (Signal-safe I/O) routines */
ssize_t sio_puts(char s[]);
//ssize_t sio_putl(long v);
void sio_error(char s[]);

void unix_error(char *msg);

handler_t *Signal(int signum, handler_t *handler);


#endif /* __MYSHELL_H__ */
/* $end myshell.h */

