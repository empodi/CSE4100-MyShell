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
#define MAXJOB 128

/* External variables */
extern int h_errno;    /* Defined by BIND for DNS errors */ 
extern char **environ; /* Defined by libc */
static bool exitFromChild = false;
static char nullCmd[MAXLINE];   // just for initializing jobList
static char originCmd[MAXLINE];

typedef void handler_t(int);

typedef enum fgbg {
    NONE, FG, BG, SUSPEND
} ground;

typedef enum st {
    RUN, STOP, DONE, DONENOSIG
} state;

typedef struct _JOB {
    int id;
    pid_t pid;
    char cmd[MAXLINE];
    int cmdLen;
    ground grd;
    state curst;
} job;

job jobList[MAXJOB];

/* Function prototypes */
void eval(char* cmdline, int *fd, char* originCmd);
int parseline(char *buf, char **argv);
int builtin_command(char **argv); 

int CD(char** argv);

void setJob(int jIdx, int id, pid_t pid, char* cmd, ground gr, state st);
void addJob(pid_t pid, char* cmd, ground gr, state st);
void delJob(int jIdx);
void updateJobCMD(int jIdx, bool ampersandFlag);
void printJob(int jIdx);
void modifiedWait(int jNum);
/* Sio (Signal-safe I/O) routines */
ssize_t sio_puts(char s[]);
//ssize_t sio_putl(long v);
void sio_error(char s[]);

void unix_error(char *msg);

handler_t *Signal(int signum, handler_t *handler);


#endif /* __MYSHELL_H__ */
/* $end myshell.h */

