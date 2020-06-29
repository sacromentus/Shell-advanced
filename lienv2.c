//	Jerry Sufleta, HW #10
//	Shell with piping and redirection

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#define BUFFER 		200
#define MAX_FLAGS 	100

/*	Constant values which parse_input will return to relay to  main()
		how to change the fd's	*/

#define	STD		2	// do not alter stdin or stdout
#define RDIR_OUT	1
#define RDIR_IN 	0
#define PIPE 		3

char CWD[BUFFER+1];
char dPath[BUFFER+1];	//

int parse_input(char *, char *[], char *);

int prompt();
void error(const char *);
void execute_command(char *[], int direction);
void extract_arguments(char *[], char *[], char *[]);
void pipedProcesses(char *[]);
void tunnel(char *[], int);


int main (int argc, char *argv[]) {
	char input[BUFFER+1];
	char *parsed[MAX_FLAGS+1];	

	strncpy(dPath, CWD, BUFFER);	// set default path
	while(prompt() && fgets(input, BUFFER, stdin) != NULL) {
	
		int direction = parse_input(input, parsed, " \t\n");
		
		if(parsed[0] == NULL)	//invalid command
			continue;
	
		if(strncasecmp("exit", parsed[0], BUFFER) == 0)
			return 0;
		
		
		else if(strncmp("cd", parsed[0], BUFFER) == 0) {
			if(parsed[1] == NULL)
				chdir(dPath);
			else
				chdir(parsed[1]);			
		}
		else 
			execute_command(parsed, direction);
			
	}

	return 0;
}

void error (const char *msg)
{
  perror (msg);
  exit (1);
}

int parse_input(char *input, char *output[], char *delim) {
	char *value;
	int direct = STD;	// by default we leave stdin & stdout as default

	value = strtok(input, delim);
	
	if(value == NULL) {
		output[0] = NULL;
		return direct;
	}

	output[0] = value;
	
	

	int i;

	for( i = 1; i < MAX_FLAGS && (value =  strtok(NULL, delim)) != NULL; ++i) {
		if(strcmp(value, "|") == 0 ) {
			direct = PIPE;
			output[i] = NULL;
		}

		else if(strcmp(value, ">") == 0) {
			direct = RDIR_OUT;
			output[i] = NULL;
		}

		else if(strcmp(value, "<") == 0) {
			direct = RDIR_IN;
			output[i] = NULL;
		}
	

		else
			output[i] = value;

	}

	output[i] = NULL;
	return direct;
}

int prompt() {
	char *result = getcwd(CWD, BUFFER);
	
	if(result == NULL)
		return 0;

	CWD[BUFFER] = '\0';
	printf("bcshell %s %% ", CWD);
	return 1;
}


void execute_command(char *command[], int direction) {
	if(direction == RDIR_IN || direction == RDIR_OUT) 
		tunnel(command, direction);

	else if(direction == PIPE)
		pipedProcesses(command);

	else {		
		int childPid = fork();
		if(childPid == 0) {
			execvp(command[0], command);
			error("child is still alive! kill it before it grows!\n");
		}

		else {
			int status;
			wait(&status);
		}
	}
}	

void pipedProcesses(char *args[]) {
	char *piper[MAX_FLAGS-1];
	char *pipee[MAX_FLAGS-1];	

	extract_arguments(args, piper, pipee);

	int fd[2];
	int k = pipe(fd);	//request location to fd's where we can pipe
	
	if(k == -1)
		error("pipe() failed!");

	int childPid = fork();	//create our first child, this kid will be 
				// our piper

	if(childPid == 0) {	//ready to pipe!
		close(fd[0]);	
		
		k = dup2(fd[1],1);

		if(k == -1)
			error("dup2() failed!");
		
		int m = execvp(piper[0], piper);
		error("execvp() failed!");
	}

	int child2Pid = fork();

	if(child2Pid == 0)	{	//pipee!
		close(fd[1]);
		k = dup2(fd[0], 0);

		if(k == -1)
			error("dup2() failed!");

		int n = execvp(pipee[0], pipee);
		error("execvp() failed!");
	}

	// Now we're the parent, so we must wait for our children
	//	to finish playing

	int status;
	int p = waitpid(childPid, &status, 0);

	if (p == -1)
		error("waitpid() failure!");

	close(fd[1]);

	p = waitpid(child2Pid, &status, 0);

	if(p == -1)
		error("waitpid() failed!");
	close(fd[0]);
}

void tunnel(char *args[], int direction) {
	int childPid = fork();

	if(childPid == 0) {
		char *prog[MAX_FLAGS-1];
		char *file[2];
		int newfd;

		extract_arguments(args, prog, file);

		if(direction == RDIR_IN)
			newfd = open(file[0], O_RDONLY, S_IRUSR);
		else
			newfd = open(file[0], O_CREAT | O_WRONLY | O_TRUNC, 0777);

		if(newfd == -1)
			error("open() failed!");

		int fdcheck = dup2(newfd, direction);

		int k = execvp(prog[0], prog);
		error("execvp() failed!");
	}
	
	else {
		int status;
		wait(&status);
	}
}

void extract_arguments(char *raw[], char *dest1[], char *dest2[]) {
	int i =0;
	int j =0;

	while( (dest1[i++] = raw[j++]) != NULL)
			;
	i = 0;

	while( (dest2[i++] = raw[j++]) != NULL)
			;

}		
	
	
