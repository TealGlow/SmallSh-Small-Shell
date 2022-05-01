/* Name: Alyssa Comstock
 * Class: CS344 - Operating Systems
 * Last Date Edited:  
 * Assignment: Smallsh.c Portfolio Assignment
 * Description: Contains a smallsh program that handles user commands
 * and runs the user command either in the foreground or background.
 * */

/* setenv(), unsetenv()  macro*/
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

// my header contains struct definition, function prototypes, and constants
#include "smallsh.h"

// foreground only check
int fg_only = 0; // 0 == bg accepted; 1 == fg only mode.
int background_term = -1; // is a process is terminated, then the status will be put here as a posi number
int activepids[100]={-1};
int num_pids = 0;

int main(){
	fprintf(stdout, "pid: %d\n", getpid());
	fflush(stdout);

	// on normal exit, clean up zombie processes
	atexit(zombie_handler);
	
	
	// init child stuff	
	int childStatus = 0;
	pid_t childPid = -5;

	// Signal handlers!
 	// Signal handler for when a child finishes:
	struct sigaction s_child;

	/*	
	sigemptyset(&s_child.sa_mask);
	s_child.sa_flags = SA_RESTART;
	s_child.sa_handler = zombie_handler;
	// sigchld from lectures - handles when child's state changes
	if(sigaction(SIGCHLD, &s_child, NULL)==-1){
		fprintf(stdout, "Error in sigaction for child processes.\n");
		fflush(stdout);
		exit(1);
	}*/
	
	// for signal handling for ctrl+c	
	//struct sigaction sa_c;
	/*
	sigemptyset(&sa_c.sa_mask);
	sa_c.sa_flags = 0;
	sa_c.sa_handler = sig_handler;
	if(sigaction(SIGINT, &sa_c, NULL) == -1){
		fprintf(stdout, "error sig_c\n");
		flushAllStreams();
		exit(1);
	}*/

	// signal handler for ctrl + z, this will flip the global variable making
	// something as foreground or background. 	
	/*struct sigaction sa_z;
	
	sigemptyset(&sa_z.sa_mask);
	sa_z.sa_flags = SA_RESTART; // flag to make reentry not interrupt user command input
	sa_z.sa_handler = sig_handlerz;
	if(sigaction(SIGTSTP, &sa_z, NULL) == -1){
		fprintf(stdout, "error sig_z\n");
		fflush(stdout);
		exit(1);
	}*/
	

	do{
	
		// struct for user input to go to
		UserArgs Args;
		
		// get user input from stdin
		int r = 1;

		while(r==1){
		//	sigsetjmp(test,0);
					
			fflush(stdin);
			clearArgs(&Args);
			r = getFullUserInput(&Args);
			r == 1  ? dealloArgs(&Args) : 1;
		}
	
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
			// post the status to the user
			fprintf(stdout,"exit value  %d\n", WEXITSTATUS(childStatus));
			fflush(stdout);
	
		}else{
			// handle other args here
			childPid = fork();
		
			
			// TODO: REMOVE LATER
			// testing: automatically kill process after 10 min
			alarm(250);		
			/* Citation for switch statement to handle forking
 			 * Date: 4/26/2022
 			 * Copied and adapted from code provided in lectures about forking
 			 * Url: https://canvas.oregonstate.edu/courses/1870063/pages/exploration-environment?module_item_id=22026550
 			 * */	
			switch(childPid){
				case -1:
					fprintf(stderr, "Failed to fork.\n");
					fflush(stderr);	

					dealloArgs(&Args);	
					exit(1);
					break;
				case 0:	
					// successful child creation

					fflush(stdout);
					fflush(stdin);
					fflush(stderr);	 // flush streams for input / output redirection
					if(handleRedirection(&Args) == 1){
						// function that redirects input and output depending
						dealloArgs(&Args);
						exit(1);
					}
							
					fflush(stdout);
					fflush(stdin);
					fflush(stderr);	

					// execute command	
					execvp(Args.args[0], Args.args);
							
					fprintf(stderr, "%s: no such file or directory\n", Args.args[0]);
					fflush(stderr);

					// mem clean up
					dealloArgs(&Args);	
					exit(2);
					break;
				default:
					// wait for child to come back
					if(Args.background == 1 && !fg_only){
						// from the lectures, use of WNOHANG for child to run in background.
						pid_t resPid = waitpid(childPid, &childStatus, WNOHANG);
						activepids[num_pids]=childPid;
						num_pids++;
						for(int i=0; i < num_pids; ++i){
							fprintf(stdout, "background: %d\n", activepids[i]);
						}
						// process set to run in the background, we are going to store the pis

						// example output says to output the child pid like this
						fprintf(stdout, "background pid is %d\n", childPid);
						fflush(stdout);
					}else{
						pid_t resPid = waitpid(childPid, &childStatus, 0);
					}


					break;
			}
				
			
	
		}

		fflush(stdout);
		// trash collection before loop continues
		dealloArgs(&Args);	
	}while(1);	
	return 0;
}



void sig_handler(int sig){
	fprintf(stdout, "terminated by signal %d\n", sig);
	fflush(stdout);
}



/* sig_handlerz(void)
 *
 * Function that handles the signal for ctrl+z
 * Toggles between background and foreground mode
 *
 * Displays the message to the user.
 * */
void sig_handlerz(){
	if(fg_only == 1){
		// exit foreground only
		fg_only = 0;
		char msg[34] = "\nExiting foreground-only mode.\n: \0";
		write(STDOUT_FILENO, msg, sizeof(msg)); 
		fflush(stdout);
	}else{
		fg_only=1;
		char msg[54] = "\nEntering foreground-only mode. (& is now ignored)\n: \0";
		write(STDOUT_FILENO, msg, sizeof(msg));
		fflush(stdout);
	}
}



/* zombie_handler
 * Function that waits for all zombie processes to end
 * and displays to the user when it does and what id ended.
 * */
void zombie_handler(){
	pid_t childPid;
	int childStatus;
	
	/* Citation for the use of waitpid
 	 * Date: 4/29/2022
 	 * Referenced and used for clean up of zombie processes
 	 * url: https://linux.die.net/man/2/waitpid
 	 * */
	// According to the linux man, pid < -1 means that 
	// "wait for any child process whose process group ID is equal to
	// the absolute valid of pid."
	//
	// "-1 meaning wait for any child process."
	while((childPid = waitpid(-1, &childStatus, WNOHANG)) > 0){
		char msg[17] = "\nBackground pid \0";
		write(STDERR_FILENO, msg, sizeof(msg));
		
		fflush(stdout);
		fflush(stderr);
		fflush(stdin);
	}
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
			fprintf(stderr, "Cannot open %s for output.\n", Args->infile);
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
			fprintf(stderr, "Cannot open %s for input.\n", Args->outfile);
			fflush(stderr);
			return 1;
		}
		fflush(stdin);
		if(fd2!=STDIN_FILENO){
			dup2(fd2, STDIN_FILENO);
		}
		close(fd2);
	}

	else if(Args->outfile[0] == '\0' && Args->infile[0] == '\0' && Args->background == 1){
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
		if(fd3 != STDOUT_FILENO || fd3!=STDIN_FILENO){
			dup2(fd3, STDOUT_FILENO);
			dup2(fd3, STDIN_FILENO);
		}
		close(fd3);
	}	
	return 0;
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
	memset(buffer, '\0', MAX_LINE_LENGTH);

	int f = 0; // 0 = args; 1 = in; 2 = out
	char t = 0;
	size_t i = 0;
	fprintf(stdout, ": ");
	fflush(stdout);


	fflush(stdin);

	do{
		t = fgetc(stdin);
		if((t == ' '&& i==0) || (t == '\n'&&i==0) || (t == '#'&&i==0)){
			// first value check
			while(t!='\n') t=fgetc(stdin);
			return 1;
		}
		// read in characters one at a time
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
		else if(t == ' '||t=='\n'){
			// t == current; buffer[i] == prev	
			buffer[i-1] == '>' ? f = 1: 1;
			buffer[i-1] == '<' ? f = 2: 1;
			buffer[i] = '\0';	

			if(f == 0 && buffer[i-1] != '>' && buffer[i-1] != '<'){
				// normal arg, add it to the char*[] arg list.	
				Args->args[Args->amount_args] = malloc(i+1);
				strncpy(Args->args[Args->amount_args], buffer, i);
				Args->args[Args->amount_args][i] = '\0';
				Args->amount_args++;
			}else if(f == 1 && buffer[0] != '\0' && buffer[0]!='>' && buffer[0]!='<' ){
				// in, add it to the infile struct obj	
				strncpy(Args->infile, buffer, i);
				Args->infile[i] = '\0';
			}else if(f == 2 && buffer[0] != '\0' && buffer[0]!='>' && buffer[0]!='<'){
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
	}while(t!='\n');
	// do something with the final character
	
	if(strcmp(Args->args[Args->amount_args-1], "&") == 0){
		// remove from Args list, and set the background flag
		free(Args->args[Args->amount_args-1]);
		Args->amount_args--;
		Args->background = 1;
	}
	Args->args[Args->amount_args] = NULL;

	if(Args->args[0][0] == '#' || Args->amount_args == 0 || Args->args[0][0] == ' '){
		return 1;
	}
		
	return 0;
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



/* dealloArgs(*Args)
 * Function that deallocates all the items in the UserArgs
 * struct object, specifically the char *[] args array.
 *
 * @param: *Args: struct object to deallocate.
 * */
void dealloArgs(UserArgs *Args){
	// frees each item in the arr that has been allocated
	if(Args->amount_args == 0) return;
	for(int i=0; i < Args->amount_args; i++) free(Args->args[i]);
}
