#include "myshell.h"

static void controlExit(int signum) {

    if (signum == SIGUSR2) {
	    exitFromChild = true;
    }
}

void SIGINTforChild(int sig){
	
    bool del = false;
    for (int i = 0 ; i < MAXJOB; i++) {

        if (jobList[i].grd == FG && jobList[i].curst != DONE) {

            kill(jobList[i].pid, SIGKILL);
            delJob(i);
            del = true;
        }
    }
    
    if (!del) {
        sio_puts("\nCSE4100-SP-P#1> ");
    }

    return;
}

void sigchld_handler(int sig) {

    pid_t rpid;
    int status;
    bool isDone = false;

    while ((rpid = waitpid(-1, &status, WNOHANG | WCONTINUED | WUNTRACED)) > 0) {

        if ((WIFEXITED(status) || WIFSIGNALED(status)) && !WIFSTOPPED(status)) {

            for (int i = 0; i < MAXJOB; i++) {
                if (jobList[i].pid == rpid) {

                    jobList[i].curst = DONENOSIG;
                    isDone = true;
                    break;
                }
            }
        }
    }

    if (isDone) {
        sio_puts("\n");
    }
    fflush(stdout);
    return;
}

void sigstop_handler(int sig) {

    for (int i = 0; i < MAXJOB; i++) {

        if (jobList[i].grd == FG && jobList[i].curst == RUN) {

            jobList[i].grd = SUSPEND;
            jobList[i].curst = STOP;

            int res = kill(jobList[i].pid, SIGTSTP);

            if (res == 0) {
                sio_puts("\n");
            }
        }
    }

    return;
}

void setJob(int jIdx, int id, pid_t pid, char* cmd, ground gr, state st) {

     if (jIdx < 0 || jIdx >= MAXJOB) {
        unix_error("Invalid Job Range \n");
        exit(1);
    }

    jobList[jIdx].id = id;
    jobList[jIdx].pid = pid;
    jobList[jIdx].grd = gr;
    jobList[jIdx].curst = st;
    
    for (int i = strlen(cmd); i >= 0; i--) {
        if (cmd[i] == '\n') {
            cmd[i] = '\0';
            break;
        }
    }

    strcpy(jobList[jIdx].cmd, cmd);
    jobList[jIdx].cmdLen = strlen(jobList[jIdx].cmd);
}

void updateJobCMD(int jIdx, bool ampersandFlag) {
    // ampersandFlag = 1 -> add ampersand
    // amdpersandFlag = 0 -> delete ampersand

    int len = jobList[jIdx].cmdLen;

    if (ampersandFlag) {
        jobList[jIdx].cmd[len] = '&';
        jobList[jIdx].cmd[len + 1] = '\0';
        jobList[jIdx].cmdLen = len + 1;
    }
    else {
        jobList[jIdx].cmd[len - 1] = '\0';
        jobList[jIdx].cmdLen = len - 1;
    }
}

void delJob(int jIdx) {

    setJob(jIdx, -1, 0, nullCmd, NONE, DONE);
}

void addJob(pid_t pid, char* cmd, ground gr, state st) {

    if (pid < 1) {
        unix_error("Invalid Process \n");
        exit(1);
    }

    int jIdx = 0;
    for ( ; jIdx < MAXJOB; jIdx++) {

        if (jobList[jIdx].id < 0) {
            setJob(jIdx, jIdx, pid, cmd, gr, st);
            break;
        }
    }

    if (jIdx == MAXJOB) {
        unix_error("Jobs Overloaded \n");
        exit(1);
    }
}

void printJob(int jIdx) {

    if (jobList[jIdx].id < 0) return;

    fprintf(stdout, "[%d] ", jobList[jIdx].id + 1);

    char* printState;

    switch(jobList[jIdx].curst) {
        case RUN:
            printState = "running"; break;
        case STOP:
            printState = "suspended"; break;
        case DONENOSIG:
        case DONE:
            printState = "done"; break;
    }

    fprintf(stdout, "%-9s", printState);

    fprintf(stdout, "  %s \n", jobList[jIdx].cmd);
}

int main() 
{
    memset(nullCmd, '\0', sizeof(nullCmd));

    for (int i = 0; i < MAXJOB; i++) {
        delJob(i);
    }

    char cmdline[MAXLINE]; /* Command line */

    Signal(SIGUSR2, controlExit);

    int fd[MAXFD * 2];
    
    do {
	    /* Read */
        Signal(SIGCHLD, sigchld_handler);
        Signal(SIGINT, SIGINTforChild);
        Signal(SIGTSTP, sigstop_handler);
        Signal(SIGTTOU, sigstop_handler);

	    printf("CSE4100-SP-P#1> ");                   
	    if (fgets(cmdline, MAXLINE, stdin) == NULL) break;
        strcpy(originCmd, cmdline);

	    if (feof(stdin)) break;

	    /* Evaluate */
	    eval(cmdline, fd, originCmd);

        for (int i = 0; i  < MAXJOB; i++) {
            if (jobList[i].curst == DONENOSIG) {
                printJob(i);
                delJob(i);
            }
        }
		
	    if (exitFromChild) {
            break;
        }
		
    } while (true);
    
    if (!exitFromChild)	
    	printf("\n");

    return 0;
}

/* $begin eval */
/* eval - Evaluate a command line */
void eval(char* cmdline, int *fd, char* originCmd)  
{
    char *argv[MAXARGS] = { 0 }; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */
    pid_t cur_pid;

    char curPipe[MAXLINE];
    char* nextPipe;

    strcpy(buf, cmdline);

    nextPipe = strchr(buf, '|');

    if (nextPipe) {

        if (pipe(fd) < 0) {
            unix_error("pipe error!! \n");
            exit(1);
        }

        nextPipe++;

        while (*nextPipe && (*nextPipe == ' ')) 
            nextPipe++;
        
        for (int i = 0; i < MAXLINE; i++) {
            if (buf[i] == '|') {
                curPipe[i] = ' ';
                curPipe[i + 1] = '\0';
                break;
            }
            curPipe[i] = buf[i];
        }

        bg = parseline(curPipe, argv);

    }
    else {
        bg = parseline(buf, argv); 
    }

    if (argv[0] == NULL)  {
	    return;   /* Ignore empty lines */
    }

    if (!builtin_command(argv)) { // & -> ignore, other -> run, execute cd

        char *basePath = "/bin/";
        char binaryPath[100];
        strcpy(binaryPath, basePath);
        strcat(binaryPath, argv[0]);

        if ((cur_pid = fork()) == 0) {

            if (bg) {//when there is background process!
                signal(SIGINT, SIG_IGN);
            }

            if (*(fd - 2)) {
                dup2(*(fd - 2), STDIN_FILENO);
            }
            if (nextPipe) {
                dup2(*(fd + 1), STDOUT_FILENO);
            }

            int exec_res = execve(binaryPath, argv, environ);

            if (exec_res < 0) {	//ex) /bin/ls ls -al &

                if (strcmp(argv[0], "exit") == 0) {

                    kill(getppid(), SIGUSR2);		   
                }
                else if (strcmp(argv[0], "jobs") == 0) {
                    
                    for (int i = 0; i < MAXJOB; i++) {
                        if (jobList[i].id > -1) {
                            printJob(i);
                        }
                    }
                }
                else {
                    fprintf(stdout, "%s: Command not found.\n", argv[0]);
                    exit(-1);
                }
            }

            exit(0);
        }

        int gr_state = (bg == 1) ? BG : FG;

        if (strcmp(argv[0], "bg") != 0 && strcmp(argv[0], "fg") != 0 && strcmp(argv[0], "jobs") != 0)
            addJob(cur_pid, originCmd, gr_state, RUN);
        
        /* Parent waits for foreground job to terminate */
        if (!bg) { 
            int status;      

            pid_t res_pid;
            if ((res_pid = waitpid(cur_pid, &status, WCONTINUED | WUNTRACED)) < 0) { // reaping
                fprintf(stderr, "%s: %s\n", "Waitpid error", strerror(errno));
                exit(-1);
            }
            else {
                
                for (int i = 0; i < MAXJOB; i++) {

                    if (jobList[i].grd == FG && res_pid == jobList[i].pid && jobList[i].curst != STOP && WIFEXITED(status)) {
                        delJob(i);
                    }
                }
            }

            if (nextPipe) {
                close(*(fd + 1));
                eval(nextPipe, fd + 2, originCmd);
            }
        }
        else { //when there is backgrount process!

            int i = 0;
            for (; i < MAXJOB; i++) {
                if (jobList[i].pid == cur_pid) {
                    break;
                }
            }
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
    if (strcmp(argv[0], "fg") == 0) {

        char tmp[10];
        strcpy(tmp, &(argv[1][1]));
        int jNum = atoi(tmp);

        jNum--;

        if (jNum < 0 || jNum >= MAXJOB || jobList[jNum].id < 0) {
            fprintf(stdout, "No Such Job \n");
            return 1;
        }

        jobList[jNum].grd = FG;
        jobList[jNum].curst = RUN;
        updateJobCMD(jNum, false);

        kill(jobList[jNum].pid, SIGCONT);
        fprintf(stdout, "%s \n", jobList[jNum].cmd);

        for (int i = strlen(jobList[jNum].cmd); i >= 0 ; i--) {
            if (jobList[jNum].cmd[i] == '&') {
                jobList[jNum].cmd[i] == '\0';
            }
        }
        
        modifiedWait(jNum);

        return 1;
    }
    if (strcmp(argv[0], "bg") == 0) {

        char tmp[10];
        strcpy(tmp, &(argv[1][1]));
        int jNum = atoi(tmp);

        jNum--;

        if (jNum < 0 || jNum >= MAXJOB || jobList[jNum].id < 0) {
            fprintf(stdout, "No Such Job \n");
            return 1;
        }

        if (jobList[jNum].grd == BG) {
            fprintf(stdout, "bg: job already in background \n");
            return 1;
        }

        jobList[jNum].grd = BG;
        jobList[jNum].curst = RUN;
        updateJobCMD(jNum, true);

        kill(jobList[jNum].pid, SIGCONT);
        return 1;
    }
    else if (strcmp(argv[0], "kill") == 0) {

        char tmp[10];
        strcpy(tmp, &(argv[1][1]));
        int jNum = atoi(tmp);

        jNum--;

        if (jNum < 0 || jNum >= MAXJOB || jobList[jNum].id < 0) {
            fprintf(stdout, "No Such Job \n");
            return 1;
        }

        int resKill = kill(jobList[jNum].pid, SIGKILL);
        
        if (resKill == 0) {
            delJob(jNum);
            return 1;
        }
        else {
            unix_error("Kill Fail\n");
        }
    }

    return 0;                     
}
/* $end eval */

int CD(char** argv) {

    char path[1024];

    if (argv[1] == NULL || !strcmp(argv[1], "~") || !strcmp(argv[1], "~/")) {

        return chdir(getenv("HOME"));
    }
    else if (!strcmp(argv[1], "..") || !strcmp(argv[1], "../")) {

        int result =  chdir("..");
        return result;
    }
    else if (argv[1][0] == '/') {
        return chdir(argv[1]);
    }

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
    int bg = 0;              /* Background job? */
    int isEchoGrep = 0;

    if (buf[strlen(buf) - 1] == '\n')
        buf[strlen(buf) - 1] = ' ';  /* Replace trailing '\n' with space */

    while (*buf && (*buf == ' ')) /* Ignore leading spaces */
	    buf++;

    // check bg
    for (int i = strlen(buf) - 1; i >= 0; i--) {

        if (buf[i] == ' ' || buf[i] == '\n') continue;

        if (buf[i] == '&') {

            bg = 1;
            buf[i] = ' ';
        }
        break;
    }

    /* Build the argv list */
    argc = 0;
    while ((delim = strchr(buf, ' '))) {

	    argv[argc++] = buf;
	    *delim = '\0';
	    buf = delim + 1;

        if (!strcmp(argv[0], "echo") || !strcmp(argv[0], "grep")) {
            isEchoGrep = 1;
            break;
        }

	    while (*buf && (*buf == ' ')) /* Ignore spaces */
            buf++;

        if (isEchoGrep) break;
    }

    if (isEchoGrep) {

        char tmp[1000];
        int t = 0;

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

        if (*buf == '&') {
            *argv[argc++] = '&';
        }
    }

    argv[argc] = NULL;

    if (argv[0] != NULL && !strcmp(argv[0], "grep")) {

        for (int i = strlen(argv[1]); i >= 0; i--) {

            if (argv[1][i] == '"' || argv[1][i] == '\'' || argv[1][i] == ' ') {
                argv[1][i] = '\0';
            }
        }
    }
    
    if (argc == 0)  /* Ignore blank line */
	    return 1;

    /* Should the job run in the background? */
    if (bg == 0) {
        if ((bg = (*argv[argc-1] == '&')) != 0) {

            // delete ampersand
            argv[--argc] = NULL;
        }
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

void modifiedWait(int jNum)
{
    while (1) {
        if (jobList[jNum].curst == STOP)
            break;
        if (jobList[jNum].curst == DONE)
            break;
        if (jobList[jNum].grd == SUSPEND)
            break;
        if (jobList[jNum].id < 0)
            break;
    }
    
    return;
}
