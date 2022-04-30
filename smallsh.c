/* Name: Alyssa Comstock
 * Class: CS344 - Operating Systems
 * Last Date Edited:  
 * Assignment: Smallsh.c Portfolio Assignment
 * */

/* setenv(), unsetenv()  macro*/
#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include "smallsh.h"


/* CONSTANTS */
/*#define  MAX_LINE_LENGTH  2048
#define MAX_ARG_NUM  512


struct userArgs{
	char * args[MAX_ARG_NUM]; // contains the args
	char infile[256]; // contains the in file supplied by the user
	char outfile[256]; // contains the outfile supplied by the user
	int background; // & symbol at end == execute in background, so background = 1. Else background = 0
	int amount_args; // For deallocation
};

typedef struct userArgs UserArgs;
*/

/* FUNCTION PROTOTPES */
/*int getFullUserInput(UserArgs *Args);
void cdAndUpdatePWD(char*);
void cleanUpProcesses();*/
/* struct specific function prototype*/
/*void displayArgs(UserArgs *Args);
void cleanUpArgs(UserArgs *Args);
void clearArgs(UserArgs *Args);
void dealloArgs(UserArgs *Args);*/

int handleRedirection(UserArgs *Args);
void flushAllStreams(void);

// globals
int pidList[25] = {-1}; // list that stores childPids, inits all of the pids to -1



int main(){
	fprintf(stdout, "pid: %d\n", getpid());
	fflush(stdout);
	atexit(cleanUpProcesses);

	// init child stuff	
	int childStatus = 0;
	pid_t childPid = -5;
	
	do{
	
		// struct for user input to go to
		UserArgs Args;
	
		// get user input from stdin
		int r = 0;
		do{
			fprintf(stdout, ": ");
			flushAllStreams();
			clearArgs(&Args);
			r = getFullUserInput(&Args);
			r == 0 ? dealloArgs(&Args) : 1;
		}while(r == 0);
		
		// testing display
		//displayArgs(&Args);

		// check first arg for exit, cd, status in a chain
		if(strcmp(Args.args[0], "exit") == 0 || strcmp(Args.args[0], "exit\n") == 0){	
			dealloArgs(&Args);
			// we are done here!
			exit(0);
		}else if(strcmp(Args.args[0], "cd") == 0){
			// cd to user defined dir in the command
			fprintf(stdout, "cd to dir: %s\n", Args.args[1]);
			fflush(stdout);
	
			cdAndUpdatePWD(Args.args[1]);
				
		}else if(strcmp(Args.args[0], "status") == 0 || strcmp(Args.args[0], "status\n") == 0){
			fprintf(stdout, "post status\n");
			fflush(stdout);
			fflush(stdin);
			fflush(stderr);	

			fprintf(stdout,"Status: %d\n", WEXITSTATUS(childStatus));

			fflush(stdout);
			fflush(stdin);
			fflush(stderr);	

		}else{
			// handle other args here
			childPid = fork();
		
			
			// TODO: REMOVE LATER
			// testing: automatically kill process after 10 min
			alarm(250);		
	
			switch(childPid){
				case -1:
					fprintf(stderr, "Failed to fork.\n");
					fflush(stdout);
					fflush(stdin);
					fflush(stderr);	

					dealloArgs(&Args);	
					exit(1);
					break;
				case 0:	


					fflush(stdout);
					fflush(stdin);
					fflush(stderr);	
					if(handleRedirection(&Args) == 1){
						dealloArgs(&Args);
						exit(1);
					}
							
					fflush(stdout);
					fflush(stdin);
					fflush(stderr);	

					// execute command	
					execvp(Args.args[0], Args.args);
					
					fprintf(stderr, "Error with cmd\n");
					fflush(stderr);

					// mem clean up
					dealloArgs(&Args);	
					exit(2);
					break;
				default:
					// wait for child to come back
					if(Args.background == 1){
						childPid = waitpid(childPid, &childStatus, WNOHANG);

						// process set to run in the background, we are going to store the pid
						for(int i=0; i<25; ++i){
							if(pidList[i] == -1){
								pidList[i] = childPid;
								break;
							}
						}

					fprintf(stdout, "pidList: \n", pidList[0]);
					flushAllStreams();

					}else{
						childPid = waitpid(childPid, &childStatus, 0);
					}


					/*if(WIFEXITED(childStatus)){
						fprintf(stdout, "Child %d exited normally with status %zu\n", childPid, WEXITSTATUS(childStatus));
						fflush(stdout);
					}else{
						fprintf(stdout,"Child %d exited abnormally due to signal %zu\n", childPid, childStatus);
						fflush(stdout);
					}*/
					break;
			}
				
			
	
		}

		fflush(stdout);
		// trash collection before loop continues
		dealloArgs(&Args);	
	}while(1);	
	return 0;
}



/* handleRedirection
 * Function that handles the redirection of stdin, stdout
 * in to files and out of files using dup2
 * If a processes needs to run in to the background, then redirection goes to
 * /dev/null and is read from /dev/null
 *
 * @param: UserArgs struct object containing the infile and outfile information
 * returns: 1 if there was an error; 0 otherwise.
 * */
int handleRedirection(UserArgs *Args){
	if(Args->infile[0] != '\0'){
		/* Citation for use of dup2 to read in stdin output into infile
		 * Author: Michael Kerrisk
		 * Book: The Linux Programming Interface
		 * Chapter: 27.4 - File Descriptors and exec()
		 * Page: 576
		 * Date: 4/27/2022
		 * Adapted frome example code on use of dup2
		 * */
		// open in file, get contents from execvp, put it in infile.
		int fd = open(Args->infile, O_WRONLY | O_CREAT | O_TRUNC, 00700);
		if(fd == -1){
			// error
			fprintf(stderr, "Cannot open %s for writing.\n", Args->infile);
			fflush(stderr);
			return 1;
		}
		fflush(stdout);
		if(fd!=STDOUT_FILENO){
			dup2(fd, STDOUT_FILENO);
		}
		close(fd);
	}

	if(Args->outfile[0] != '\0'){
		/* Citation for use of dup2 to read in stdin output into infile
 		* Author: Michael Kerrisk
 		* Book: The Linux Programming Interface
 		* Chapter: 27.4 - File Descriptors and exec()
 		* Page: 576
 		* Date: 4/27/2022
 		* Adapted frome example code on use of dup2
 		* */

		// if there is an outfile
		int fd2 = open(Args->outfile, O_RDONLY);
		if(fd2 == -1){
			// error
			fprintf(stderr, "Cannot open %s for reading.\n", Args->outfile);
			fflush(stderr);
			return 1;
		}
		fflush(stdin);
		if(fd2!=STDIN_FILENO){
			dup2(fd2, STDIN_FILENO);
		}
		close(fd2);
	}

	/*else if(Args.outfile[0] == '\0' && Args.infile[0] == '\0' && Args.background == 1){
		// Redirect all input through
		fflush(stdin);
		fflush(stdout);
		int fd3 = open("/dev/null", 0);
		if(fd3 == -1){
			fprintf(stderr, "Cannot open /dev/null for writing.\n");
			fflush(stderr);	
			return 1;
		}
		fflush(stdout);
		if(fd3 != STDOUT_FILENO){
			dup2(fd3, STDOUT_FILENO);
		}
		close(fd3);
	}	*/
	return 0;
}



/* flushAllStreams
 * Function that fflush's stdin, stdout, and stderr
 * */
void flushAllStreams(){
	fflush(stdin);
	fflush(stdout);
	fflush(stderr);
}



/* Function that displays the args in the struct
 * shows if process is supposed to run in the background
 * shows infiles and outfiles
 * */
void displayArgs(UserArgs *Args){

	fprintf(stdout, "=====PRINTING ARGS=====\n");
	fflush(stdout);
	// testing prints
	if(Args->args){
		for(int j = 0; j<Args->amount_args; ++j){
			fprintf(stdout,"arg: %s\n", Args->args[j]);
			fflush(stdout);
		}
	}
	if(Args->infile){
		fprintf(stdout, "infile: %s\n", Args->infile);
		fflush(stdout);
	}
	if(Args->outfile){
		fprintf(stdout, "outfile: %s\n", Args->outfile);
		fflush(stdout);
	}
	if(Args->background){
		fprintf(stdout, "background: %d\n", Args->background);
		fflush(stdout);
	}
	return; 
}



/* Function that handles changing directories
 * If no directory to go to is give, go to home directory 
 * from environment variables
 *
 * @param: toGoTo: absolute / relative path to change to.
 * */
void cdAndUpdatePWD(char * toGoTo){
	if(toGoTo == NULL){
		// if no arg to cd to, cd to HOME env val
		toGoTo = getenv("HOME");
	}

	char buffer[256];
	int res = chdir(toGoTo);
	
	if(res == 0){
		// we successfully changed dir, need to update pwd
		getcwd(buffer, 255);
		buffer[strlen(buffer)] = '\0';
		setenv("PWD", buffer, 1);
	}else{
		// error message but its not enough to exit
		fprintf(stderr, "Error with cd: dir does not exist\n");
		fflush(stderr);
	}
}



/* getFullUserInput(*Args)
 * Function that reads the user input from the command line,
 * and returns to main 0 if there was no issue.
 *
 * @params: Args; struct object to store the user input
 * returns success or failure int
 * */
int getFullUserInput(UserArgs *Args){
	// read in character one at a time, so we need a temp buffer
	char buffer[MAX_LINE_LENGTH+1];
	int f = 0; // 0 = args; 1 = in; 2 = out
	char t = 0;
	size_t i = 0;
	do{
		// read in characters one at a time
		t = fgetc(stdin);
		if(t == '$' && buffer[i-1] == '$'){
			// variable expansion, if current == $, change prev to be the pid 
			// and skip adding this one
	
			/* Citation for using snprintf to convert an int to a string
 	 		 * Date: 4/28/2022
 	 		 * Adapted from code provided by the professor in the ed discussion
 	 		 * Url: https://edstem.org/us/courses/21025/discussion/1437434?answer=3250618
 	 		 * */

			pid_t pid = getpid();
			int length = snprintf(NULL, 0, "%jd", pid);
			char *p;
			p = malloc(length+1); 
			snprintf(p, length+1, "%jd", pid);
			i--; // go back 1 to change the previous $ into the pid
			for(int j=0; j<length; j++){
				// because string pid is going to be of any length we
				// add each new pid string char to the buffer and add 1
				buffer[i] = p[j];
				i++; // add up how many char we added
			}
			free(p);

		}
		else if(t == ' ' || t=='\n'){
			// t == current; buffer[i] == prev	
			buffer[i-1] == '>' ? f = 1: 1;
			buffer[i-1] == '<' ? f = 2: 1;
			buffer[i] = '\0';	
			if(f == 0 && buffer[i] != '>' && buffer[i] != '<'){
				// normal arg, add it to the char*[] arg list.	
				Args->args[Args->amount_args] = malloc(i+1);
				strncpy(Args->args[Args->amount_args], buffer, i);
				Args->args[Args->amount_args][i] = '\0';
				Args->amount_args++;
			}else if(f == 1 ){
				// in, add it to the infile struct obj	
				strncpy(Args->infile, buffer, i);
				Args->infile[i] = '\0';
			}else if(f == 2 ){
				strncpy(Args->outfile, buffer, i);
				Args->outfile[i] = '\0';
			}
			memset(buffer, '\0', MAX_LINE_LENGTH);
			i = 0;

		}else{
			//get buffer until u
			buffer[i] = t;
			i++;
		}

	}while(t!=EOF && t!='\n');

	if(Args->amount_args == 0){
		return 0;
	}	
	if(strcmp(Args->args[Args->amount_args-1], "&") == 0){
		// remove from Args list, and set the background flag
		free(Args->args[Args->amount_args-1]);
		Args->amount_args--;
		Args->background = 1;
	}
	Args->args[Args->amount_args] = NULL;

	if(strcmp(Args->args[0], "#") == 0){
		return 0;
	}
		
	return 1;
}




/* clearArgs(*Args)
 * Function that resets all the varaibles in the UserArgs object struct
 * Gets the struct and sets all the values to 0, NULL or \0 when needed
 *
 * @params: Args: struct object to clear.
 * Returns: nothing
 * */
void clearArgs(UserArgs *Args){
	
	// resetting vars
	memset(Args->infile, '\0', sizeof(char)*MAX_ARG_NUM);
	memset(Args->infile, '\0', sizeof(char)*MAX_ARG_NUM);
	for(int i=0; i<MAX_ARG_NUM; ++i) Args->args[i] = NULL;
	Args->background = 0;
	Args->amount_args = 0;

}



/* cleanUpArgs(*Args)
 * Function that deallocates all the items in the UserArgs
 * struct object, specifically the char *[] args array.
 *
 * @param: *Args: struct object to deallocate.
 * */
void dealloArgs(UserArgs *Args){
	// frees each item in the arr that has been allocated
	for(int i=0; i < Args->amount_args; i++) free(Args->args[i]);
}



/* cleanUpProcesses()
 * function that cleans up any extra zombie
 * processes that might have been left behind
 * this function is automatically called at the end of
 * normal program exit with atexit
 * */
void cleanUpProcesses(){
	/*int z;
	do{
		z = wait(NULL);
		fprintf(stdout, "Freed: %d\n", z);
		fflush(stdout);
	}while(z != -1);*/
	int j = -1;
	int childStatus;
	for(int i=0; i<25; ++i){
		j = waitpid(pidList[i], &childStatus, 0); 
		fprintf(stdout, "Freed: %d\n", j);
		flushAllStreams();
	}

}
