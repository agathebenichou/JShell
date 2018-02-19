/* $begin shellmain */
#include "csapp.h"
#include <string.h>
#include <stdlib.h>
#include <sys/resource.h>
#define MAXARGS   128

/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv); 
void statistics();
void SIGINThandler(int sigNumber);
void SIGTSTPhandler(int sigNumber);
 
/* Should the job run in bg or fg? */
int bg;           

/* The character array that stores enabled statistics */
char enabledStats[1] = "";
int statsCounter = 0;

/* Global variables to keep tracking of PIDS */
static sig_atomic_t foregroundPID;
static sig_atomic_t pidArray[1];
int pidCounter = 0;

/* Global variables to keep tracking of Jobs */
struct Job {
    int pid;
};
struct Job totalJobs[10];
int jobCounter=0;

/* $begin shellmain */
/* Main method */
int main() 
{
    char cmdline[MAXLINE]; /* Command line */

    /* Catch signals */
    signal(SIGINT, SIGINThandler);  
    signal(SIGTSTP, SIGTSTPhandler);  

    while (1) {
	/* Read */
    	printf("lsh> ");                 
    	fgets(cmdline, MAXLINE, stdin); 
    	if (feof(stdin))
    	    exit(0);

    	/* Evaluate */
    	eval(cmdline);
    } 
}
/* $end shellmain */
  
/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline) 
{
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int lastFiles[2] = {0,0}; /* stdin*/
    int currentFiles[2] = {0,0}; /* stdout */
    pid_t pid;           /* Process id */
    int firstarg = 0;
	int lastarg = 0;

    strcpy(buf, cmdline);
    bg = parseline(buf, argv); 
    if (argv[0] == NULL)  
	return;   /* Ignore empty lines */
   
    /* Determine the number of args passed to the shell */
    int argLength=0;
    while(argv[++argLength] != NULL);

    /* Check if the command is built in */
    if (!builtin_command(argv)) { 

        /* Run statistics if command is not built in */
        statistics(RUSAGE_SELF);

    	/* Check and count the command for a pipe symbol */
    	int countPipe = 0;
	    for(int i = 0; i < argLength; i++){
	        if(!strcmp(argv[i], "|")){
	        	countPipe++;
	        }
	    } 

	    /* If there are not any pipes in the command*/
	    if(countPipe==0){
	    	if((pid = fork()) == 0) {
	        	if (execvp(argv[0], argv) < 0) {
	                printf("%s: Command not found.\n", argv[0]);
	                exit(0);
	        	}
    		}
    		return;
	    }

	    /* For every pipe, allocate stdout and stdin*/
	    for(int x = 0; x <= countPipe; x++) {

	    	/* Pipe to current files (stdout) */
	    	pipe(currentFiles);   

	    	/* Count the location of the current pipe and replace it with 0 */
	    	while (argv[lastarg] && strcmp(argv[lastarg], "|")) {
    			lastarg++;
    		}
    		argv[lastarg]=0;

    		/* Fork a child from parent process*/
	        if ((pid = fork()) == 0) {   /* Child runs user job */

    			/* If the last files (stdin) exists, use them as current stdin*/
		        if (lastFiles[0] != 0) {
		        	dup2(lastFiles[0],STDIN_FILENO);
		        	close(lastFiles[1]);
		        }

		        /* If there is another pipe after this one, pass input to stdout*/
		        if (x+1 <= countPipe) { 
	    			dup2(currentFiles[1], STDOUT_FILENO);

	    		/* Close stdout */
	    		} else {
	    			close(currentFiles[1]);
	    		}

	    		/* Close stdin (we have stdin from lastfiles) */
	    		close(currentFiles[0]); 

	    		/* Exec the child process using its args */
	            if (execvp(argv[firstarg], &argv[firstarg]) < 0) {
	                printf("%s: Command not found.\n", argv[firstarg]);
	                exit(0); 
	            }
	        }

	        /* If last files (Stdin) still exists, close them */
	        if (lastFiles[0] != 0) {
	        	close(lastFiles[0]);
	        	close(lastFiles[1]);
	        }

	        /* Set last files (stdin) to current files (stdout) */
	        lastFiles[0] = currentFiles[0];
	        lastFiles[1] = currentFiles[1];

	        /* Set first arg to the last one plus one to set the next iteration for the next pipe*/
            firstarg = lastarg+1;
            lastarg++;
		}

		/* If current files (Stdout) exists, close them*/
		if (currentFiles[0] != 0) {
			close(currentFiles[0]);
	    	close(currentFiles[1]);
		}

        /* Create Job struct, add current process PID to Job struct, add Job struct to Job array*/
        struct Job tempJob;
        tempJob.pid = pid;
        totalJobs[jobCounter] = tempJob; 
        jobCounter++;

		/* Parent waits for foreground job to terminate */
		if (!bg) {
		    int status;

            /* Add pid to pid array */
            foregroundPID = pid;
            pidArray[pidCounter] = foregroundPID;
            pidCounter++;

		    if (waitpid(pid, &status, 0) < 0)
			printf("waitfg: waitpid error");
		}
		else
		    printf("%d %s", pid, cmdline);

	}
	return;
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv) {

    /* Determine the number of args passed to the shell */
    int argLength=0;
    while(argv[++argLength] != NULL);

    /* If command is quit or q, exit the shell */
    if ((!strcmp(argv[0], "quit")) || (!strcmp(argv[0], "q"))){
    	exit(0);
    }

    /* If the command line ends with an ampersand, then the shell runs the job in
    the background. Otherwise, the shell runs the job in the foreground. */
    if ((!strcmp(argv[0], "&")) || (!strcmp(argv[0], "bg"))) {
        bg = 1;
        return 1;
    }

    /* If the command is set to foreground, set it to foregound */
    if (!strcmp(argv[0], "fg")){
        bg = 0;
        return 1;
    }

    /* If the job command is run, check the given pid to kill it and switch to the opposite plane (background or foreground) */
    if(!strcmp(argv[0], "job")){
        char *fpid = argv[1];
        int tempid = (intptr_t)fpid;
        pid_t tfpid = tempid;
        kill(tfpid, SIGCONT);
        bg = !bg;
        return 1;
    }

    /* the jobs built in command lists all background jobs */
    if (!strcmp(argv[0], "jobs")){

        /* if there are no jobs, print out disclaimer */
        if(jobCounter==0){
            printf("No jobs running\n");

        /* if there are jobs, go through and print each*/
        } else {
            for(int j = 0; j < jobCounter; j++){
               struct Job jobStr = totalJobs[j];

               printf("[%d] %d Running &\n", j , jobStr.pid );
            }
        }
         return 1;
    }

    /* Check the command for a dollar sign, which indicates that the shell should print the value
    of the environment variable defined after the dollar sign. */
    if(strstr(argv[0], "$")){ 

        /* Determine the char that the $ sign is at */ 
        char *positionOfDollar;
        positionOfDollar = malloc(strlen(argv[0])+1);
        positionOfDollar = strchr(argv[0], '$');

        /* Copy the characters following the $ sign to get the name of the environment variable */  
        char *variableName;
        variableName = malloc(strlen(argv[0])+1);
        memcpy(variableName, argv[(int)(positionOfDollar-argv[0])]+1, (strlen(argv[0])-(int)(positionOfDollar-argv[0])));

        /* Search the environment of the calling process for value associated with environment variable name */  
        char *variableValue;
        variableValue = malloc(strlen(argv[0])+1);
        variableValue = getenv(variableName);

        printf("%s\n", variableValue);

         return 1;
    }

    /* If command starts with echo, print out the following arguments */
    if(!strcmp(argv[0], "echo")){

        /* If the second argument is explicitly path, print  out the path of the current home directory */
        if(!strcmp(argv[1],"$PATH")) {

            const char *name = "HOME";
            char *val = "";
            val = getenv(name);

            printf("%s\n",val);
            return 1;
        }

        /* Go through the subsequent arguments and prints them out */
        for(int i = 1; i < argLength; i++){

            /* If there is a $ sign in the arguments,  */
            if(strstr(argv[i], "$")) {

                /* find the position of the dollar sign */
                char *dollarEcho;
                dollarEcho = malloc(strlen(argv[i])+1);
                dollarEcho = strchr(argv[i], '$');

                /* Copy whats after the dollar sign into a variable to depict the variable name */
                char *echoVar;
                echoVar = malloc(strlen(argv[i])+1);
                memcpy(echoVar, argv[i]+(int)(dollarEcho-argv[i])+1, (strlen(argv[i])-(int)(dollarEcho-argv[i])));

                /* Print out the value of the environment variable */
                printf("%s ",getenv(echoVar));

            /* If there is no $ sign in arguments, print out subsequent variables */
            } else {
                printf("%s", argv[i]);
            }
        }
        printf("\n");
        return 1;
    }

    /* If command starts with stats, enable the following statistics*/
    if(!strcmp(argv[0], "stats")){
        char tempStats[100] = "";

        /* Append the following statistics into a char array */
        for(int i = 1; i < argLength; i++){
            strncat(tempStats, argv[i], strlen(argv[i]));
        }

        /* For every char in the temp char array*/
        int tempLength = strlen(tempStats);
        for(int i = 0; i < tempLength; i++){

        	/* If the char is not a -, append it into the global enabled stats char array*/
            char *ptr = tempStats;
            if(ptr[i] != ('-')){
                enabledStats[statsCounter] = ptr[i];
                statsCounter++;
            }
        }

        printf("Currently enabled statistics: %s\n", enabledStats);
        return 1;
    }

    /*  If command contains an = sign, set the environment variable */
    if(strstr(argv[0], "=")){

    	/* Find char where the = sign is and its index*/
        char *positionOfEqual;
        positionOfEqual = malloc(strlen(argv[0])+1);
        positionOfEqual = strchr(argv[0], '=');

        /* Copy whats before the = sign into a variable to depict the variable name */
        char *envVarName;
        envVarName = malloc(strlen(argv[0])+1);
        memcpy(envVarName, argv[0], (int)(positionOfEqual-argv[0]));

        /* Copy what is after the = sign into a variable to depict the assigned value */
        char *envVarVal;
        envVarVal = malloc(strlen(argv[0])+1);
        memcpy(envVarVal, argv[0]+(int)(positionOfEqual-argv[0])+1, (strlen(argv[0])-(int)(positionOfEqual-argv[0])));

        char *success = getenv(envVarName);

        /* If success is null, there is no environment variable with that name */
        if(success == NULL){

            /* Set this variable and value to the environment*/
            setenv(envVarName, envVarVal, 0);
            printf("New environment variable %s=%s\n", envVarName, envVarVal);

        /* If the success is not null, there is an environent variable already set with that name*/
        } else {

            /* Remove this variable and value to the environment*/
            unsetenv(envVarName);
            printf("Removing the environment variable %s\n", envVarName);
        }

        return 1;
    }

    return 0;                     /* Not a builtin command */
}
/* $end eval */

/* Catch the SIGINT signal, prevent it from running and forward it to every process
in the foreground process group*/
void SIGINThandler(int sigNumber){

    /* replace sigint signal with ignore signal*/
    signal(SIGINT, SIG_IGN);
    printf(" JShell cannot be terminated using Ctrl^C\n");

    /* for each currently running process, kill it*/
    for(int i = 0; i < sizeof(pidArray); i++){
        sig_atomic_t *tempfpid = pidArray;
        sig_atomic_t fpid = tempfpid[i];

        kill(fpid, SIGINT);
    }
}

/* Catch the SIGTSTP signal, prevent it from running and forward it to every process
in the foreground process group*/
void SIGTSTPhandler(int sigNumber){

    /* replace sigint signal with ignore signal*/
    signal(SIGTSTP, SIG_IGN);
    printf(" JShell cannot be stopped using Ctrl^Z\n");

     /* for each currently running process, kill it*/
    for(int i = 0; i < sizeof(pidArray); i++){
        sig_atomic_t *tempfpid = pidArray;
        sig_atomic_t fpid = tempfpid[i];

        kill(fpid, SIGINT);
    }
}

/* statistics goes through the global char array that stores enabled statistics and prints out the 
result of those enabled statistics in the same order. */
/* $begin statistics */
void statistics(int self)
{
    if(!enabledStats[0]=='\0'){
        /*int who = RUSAGE_SELF;*/
        struct rusage usage;
        int ret;

        ret = getrusage(self, &usage);
        int statLength = strlen(enabledStats);
        printf("----------------------Statistics----------------------\n");

        /* If -c has been enabled with the stats command, clears all currently enabled stats */
        for(int i = 0; i < statLength; i++){

            /* Get pointer to enabled stats array and get the first char */
            char *ptr = enabledStats;
            char currentStat = ptr[i];

            /* If the char is a c, go through the enables stats array and clear all chars */
            switch(currentStat){
                case 'c':
                    for(int j = 0; j < statLength; j++){
                        ptr[j] = '\0';
                        enabledStats[j] = ptr[j];

                    }
                    break;
            }
        }

        /* Go through the enables stats array and check each for prints*/
        for(int i = 0; i < statLength; i++){

            /* Get pointer to enabled stats array and get the first char */
            char *ptr = enabledStats;
            char currentStat = ptr[i];

            /*  */
            switch(currentStat){

                /* -a means print out all of the above statistics */
                case 'a':
                    /* could this just call the other cases? */
                    printf("CPU time spent in user mode: %d.%06ld sec\n", usage.ru_utime.tv_sec, usage.ru_utime.tv_usec);
                    printf("CPU time spent in system/kernel mode: %d.%06ld sec\n", usage.ru_stime.tv_sec, usage.ru_stime.tv_usec);
                    printf("Hard page faults: %d\n", usage.ru_majflt);
                    printf("Voluntary context switches: %d\n", usage.ru_nvcsw);
                    printf("Involuntary context switches: %d\n", usage.ru_nivcsw);
                    printf("-------------------------------------------------------\n");

                    return;
                    break;

                /* -u means print out the cpu time spent in user mode */
                case 'u':
                    printf("CPU time spent in user mode: %d.%06ld sec\n", usage.ru_utime.tv_sec, usage.ru_utime.tv_usec);
                    break;

                /* -s means print out the cpu time spent in system/kernel mode */
                case 's':
                    printf("CPU time spent in system/kernel mode: %d.%06ld sec\n", usage.ru_stime.tv_sec, usage.ru_stime.tv_usec);
                    break;

                /* -p means print out the hard page faults */
                case 'p':
                    printf("Hard page faults: %d\n", usage.ru_majflt);
                    break;

                /* -v means print out the voluntary context switches */
                case 'v':
                    printf("Voluntary context switches: %d\n", usage.ru_nvcsw);
                    break;

                /* -i means print out the involuntary context switches */
                case 'i':
                    printf("Involuntary context switches: %d\n", usage.ru_nivcsw);
                    break; 

                 /* -l means print out a list of stats that are enabled */
                case 'l':
                   printf("List of enabled stats: %s\n", enabledStats);
                   break;
            }
        }
        printf("-------------------------------------------------------\n");
    }
}
/* $end statistics */


/* $begin parseline */
/* parseline - Parse the command line and build the argv array */
int parseline(char *buf, char **argv) 
{
    char *delim;         /* Points to first space delimiter */
    int argc;            /* Number of args */
    int bg;              /* Background job? */

    buf[strlen(buf)-1] = ' ';  /* Replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* Ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;
    while ((delim = strchr(buf, ' '))) {
	argv[argc++] = buf;
	*delim = '\0';
	buf = delim + 1;
	while (*buf && (*buf == ' ')) /* Ignore spaces */
            buf++;
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* Ignore blank line */
	return 1;

    /* Should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0)
	argv[--argc] = NULL;

    return bg;
}
/* $end parseline */

