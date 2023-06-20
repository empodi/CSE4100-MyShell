#include "myshell.h"

static void controlExit(int signum) {

    if (signum == SIGUSR2) {
        while (waitpid(-1, NULL, WNOHANG) > 0) ;
	    exitFromChild = true;
    }
}

void SIGINTforChild(int sig){
	
    fflush(stdout);
    sio_puts("\nCSE4100-SP-P#1> ");
}
/*
void SIGINTforMain(int sig) {

    sio_puts("\nCSE4100-SP-P#1> "); 
	fflush(stdout);
    signal(SIGINT, SIG_IGN);
}
*/
int main() 
{
    char cmdline[MAXLINE]; /* Command line */

    //Signal(SIGINT, SIGINTforMain);
    Signal(SIGUSR2, controlExit);
    Signal(SIGINT, SIGINTforChild);

    do {
	    /* Read */
	    printf("CSE4100-SP-P#1> ");                   
	    if (fgets(cmdline, MAXLINE, stdin) == NULL) break;

	    if (feof(stdin)) break;

	    /* Evaluate */
	    eval(cmdline);
		//fprintf(stdout, "eval done \n");
	    //printf("exitFromChild:%d \n ", exitFromChild);
	    if (exitFromChild) break;
    } while (true);
    
    if (!exitFromChild)	
    	printf("\n");

    return 0;
}


/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline) 
{
    char *argv[MAXARGS] = {0}; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */

    strcpy(buf, cmdline);
    bg = parseline(buf, argv); 

    if (argv[0] == NULL)  {
	    return;   /* Ignore empty lines */
    }

    if (!builtin_command(argv)) { //exit -> exit(0), & -> ignore, other -> run, execute cd

        char *basePath = "/bin/";
        char binaryPath[100];
        strcpy(binaryPath, basePath);
        strcat(binaryPath, argv[0]);

        if ((pid = fork()) == 0) {
            
            int res = execve(binaryPath, argv, environ);

            if (res < 0) {	//ex) /bin/ls ls -al &

                if (strcmp(argv[0], "exit") == 0) {
                    kill(getppid(), SIGUSR2);
                }
                else {
                    fprintf(stdout, "%s: Command not found.\n", argv[0]);
                    exit(-1);
                }
   
            }
            exit(0);
        }

        /* Parent waits for foreground job to terminate */
        if (!bg) { 
            int status;      

            if (waitpid(pid, &status, 0) < 0) { // reaping
                fprintf(stderr, "%s: %s\n", "Waitpid error", strerror(errno));
                exit(0);
            }
            
        }
        else { //when there is backgrount process!
            printf("%d %s", pid, cmdline);
        }
    }

    return;
}


int builtin_command(char **argv) 
{   
    if (!strcmp(argv[0], "&"))    /* Ignore singleton & */
	    return 1;

    if (!strcmp(argv[0], "cd")) {   // cd should be exectued in parent process
        // build change directory function
        if (CD(argv) < 0) {
            fprintf(stderr, "-myShell: %s: %s: %s\n", "cd", argv[1],  strerror(errno));
        }

        return 1;
    }

    /* Not a builtin command */
    return 0;                     
}
/* $end eval */

int CD(char** argv) {

    // chdir returns 0 when successful ... -1 indicates error
    char path[1024];

    if (argv[1] == NULL || !strcmp(argv[1], "~") || !strcmp(argv[1], "~/")) {
        return chdir(getenv("HOME"));
    }
    else if (!strcmp(argv[1], "..") || !strcmp(argv[1], "../")) {
        int result =  chdir("..");
        //printf("chdir(..) Result: %d \n", result);
        return result;
    }
    else if (argv[1][0] == '/') {
        return chdir(argv[1]);
    }

    //printf("NORMAL PATH!! \n");
    if (!getcwd(path, sizeof(path))) {
        fprintf(stderr, "current working directory get error: %s\n", strerror(errno));
        return -1;
    }

    strcat(path, "/");
    return chdir(strcat(path, argv[1]));
}

/* $begin parseline */
/* parseline - Parse the command line and build the argv array */
int parseline(char *buf, char **argv) 
{
    char *delim;         /* Points to first space delimiter */
    int argc;            /* Number of args */
    int bg;              /* Background job? */
    int isEcho = 0;

    buf[strlen(buf) - 1] = ' ';  /* Replace trailing '\n' with space */

    while (*buf && (*buf == ' ')) /* Ignore leading spaces */
	    buf++;

    /* Build the argv list */
    argc = 0;
    while ((delim = strchr(buf, ' '))) {

	    argv[argc++] = buf;
	    *delim = '\0';
	    buf = delim + 1;

        if (!strcmp(argv[0], "echo")) {
            isEcho = 1;
            break;
        }

	    while (*buf && (*buf == ' ')) /* Ignore spaces */
            buf++;

        if (isEcho) break;
    }

    if (isEcho) {

        char tmp[1000];
        int t = 0;

        //printf("Iniside IsEcho \n");
        while (*buf && (*buf == ' ')) /* Ignore spaces */
            buf++;

        if (*buf == '"' || *buf == '\'') buf++;
        
        while (*buf) {
            
            if (*buf == '&') {
                break;
            }

            tmp[t++] = *buf;
            buf++;
        }

        tmp[t - 1] = '\0';
        if (tmp[t - 2] == '"' || tmp[t - 2] == '\'') tmp[t - 2] = '\0';

        argv[argc++] = tmp;
        
        //printf("argv[1]: %s \n", argv[1]);
        if (*buf == '&') {
            *argv[argc++] = '&';
        }
    }

    argv[argc] = NULL;
    
    if (argc == 0)  /* Ignore blank line */
	    return 1;

    /* Should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0) {

        // delete ampersand
	    argv[--argc] = NULL;
    }

    return bg;
}
/* $end parseline */

static size_t sio_strlen(char s[])
{
    int i = 0;

    while (s[i] != '\0')
        ++i;
    return i;
}
/* $end sioprivate */

/* Public Sio functions */
/* $begin siopublic */

ssize_t sio_puts(char s[]) /* Put string */
{
    return write(STDOUT_FILENO, s, sio_strlen(s)); //line:csapp:siostrlen
}

void sio_error(char s[]) /* Put error message and exit */
{
    sio_puts(s);
    _exit(1);                                      //line:csapp:sioexit
}
/* $end siopublic */

/* $begin sigaction */
handler_t *Signal(int signum, handler_t *handler) 
{
    struct sigaction action, old_action;

    action.sa_handler = handler;  
    sigemptyset(&action.sa_mask); /* Block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* Restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
	unix_error("Signal error");
    return (old_action.sa_handler);
}
/* $end sigaction */


void unix_error(char *msg) /* Unix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}
/* $end unixerror */
