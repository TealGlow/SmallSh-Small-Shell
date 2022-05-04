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

void printUserArgs(UserArgs *Args);
void sigtstp_handler1(int sig);
void sigtstp_handler0(int sig);
void sigint_handler(int sig);

// foreground only check
volatile sig_atomic_t fg_only = 0; // 0 == bg accepted; 1 == fg only mode.
int activepids[CHILD_PROCESS_CAP];
int num_active_processes=0;
int childStatus = 0;
pid_t childPid = -5;


// signals
struct sigaction sa_z={0};
struct sigaction sa_c={0};



int main(){
	fprintf(stdout, "pid: %d\n", getpid());
	fflush(stdout);	

	for(int i=0; i<CHILD_PROCESS_CAP; ++i) activepids[i]=-1; // set up this arr
	atexit(theProcessReaper);
	//signals
	
	// ctrl+z foreground toggle
	sigemptyset(&sa_z.sa_mask);
	sa_z.sa_handler = sigtstp_handler1;
	sa_z.sa_flags = SA_RESTART;
	if(sigaction(SIGTSTP, &sa_z, NULL) == -1){
		// error
		fprintf(stderr, "Error with sigaction for ctrl+z\n");
		fflush(stderr);
	}
	
	// ctrl+c 
	sigemptyset(&sa_c.sa_mask);
	sa_c.sa_handler = SIG_IGN;
	sa_c.sa_flags = 0;
	if(sigaction(SIGINT, &sa_c, NULL)==-1){
		// error
		fprintf(stderr, "Error with sigaction for ctrl+c\n");
		fflush(stderr);

	}
	
	//setSignals();
	do{
		
		if(num_active_processes > 0 && fg_only == 1){
			// need to check on those processes?
			checkBackgroundProcesses();
		}
		// struct for user input to go to
		UserArgs Args;
		clearArgs(&Args);
		if(fg_only == 0){
			sa_z.sa_handler = sigtstp_handler1;
			sigaction(SIGTSTP, &sa_z, NULL);
		}else{
			sa_z.sa_handler = sigtstp_handler0;
			sigaction(SIGTSTP, &sa_z, NULL);
		}
		// get user input from stdin
		int r = 1;
		while(r==1){
			fflush(stdin);
			fflush(stdout);
			fflush(stderr);

			clearArgs(&Args);
			
			r = getFullUserInput(&Args);
			r == 1  ? dealloArgs(&Args) : 1;
		}
	
		//printUserArgs(&Args);	
		// check first arg for exit, cd, status in a chain
		if(strcmp(Args.args[0], "exit") == 0 || strcmp(Args.args[0], "exit\n") == 0){	
			dealloArgs(&Args);
			// get rid of zombies
			theProcessReaper();	
			// we are done here!
			exit(0);
			return 0;
		}else if(strcmp(Args.args[0], "cd") == 0){
			// cd to user defined dir in the command
			cdAndUpdatePWD(Args.args[1]);
		}else if(strcmp(Args.args[0], "status") == 0 || strcmp(Args.args[0], "status\n") == 0){
			// post the status to the user
			printChildStatus();
		}else{
			// handle other args here
		
			childPid = fork();
			alarm(250);			
			// pause SIGTSTP
			sa_z.sa_handler = SIG_DFL;
			sigaction(SIGTSTP, &sa_z, NULL);

			if(Args.background == 0){

				// accept SIGINT
				sa_c.sa_handler = sigint_handler;
				sigaction(SIGINT, &sa_c, NULL);

			}

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
						
					fflush(stdin);
					fflush(stdout);
					fflush(stderr);
	
					// successful child creation
					if(handleRedirection(&Args) == 1){
						// function that redirects input and output depending
						dealloArgs(&Args);
						exit(1);
					}	
					fflush(stdin);
					fflush(stdout);
					fflush(stderr);

					// execute command
					execvp(Args.args[0], Args.args);			
					fprintf(stderr, "%s: no such file or directory\n", Args.args[0]);
					fflush(stderr);

					// mem clean up
					dealloArgs(&Args);
					exit(1);
				default:
					// parent
					if(Args.background == 1 && fg_only == 0){ // if background 	
						fprintf(stdout, "background pid is %d\n", childPid);
						fflush(stdout);
						// Add the childPid to the list of active background pids

						addToActivePidList(childPid);
						childPid = (childPid, &childStatus, WNOHANG);			
					}else{
						// wait for process to be done.
						childPid = waitpid(childPid, &childStatus, 0);
					}
					// check if a child has returned
					if(WEXITSTATUS(childStatus) == 1){
						// kill child
						kill(childPid, -1);
					}	
					// accept SIGINT
					sa_c.sa_handler = SIG_IGN;
					sigaction(SIGINT, &sa_c, NULL);

					break;	
				}			
	
		}
		
		// check if a child is done
		checkBackgroundProcesses();
	
		// trash collection before loop continues
		dealloArgs(&Args);
	}while(1);	
	return 0;
}


void sigint_handler(int sig){
	char msg[26] = "\nTerminated by signal 2.\n\0";
	write(STDOUT_FILENO, msg, sizeof(msg));
	kill(childPid, -1);
}


// sets fg_mode to 0
void sigtstp_handler0(int sig){
	char msg[33] = "\nExiting foreground-only mode\n: \0";
	write(STDOUT_FILENO, msg, sizeof(msg));
	fg_only = 0;
}



// sets fg_mode to 1
void sigtstp_handler1(int sig){
	char msg[53] = "\nEntering foreground-only mode (& is now ignored)\n: \0";
	write(STDOUT_FILENO, msg, sizeof(msg));
	fg_only = 1;
}



/* theProcessReaper
 *
 * Function that goes through the list of activepids and kills them
 * this is done before the exit so that it force closes all the 
 * active processes.
 * */
void theProcessReaper(void){
	for(int i=0; i<CHILD_PROCESS_CAP; i++){
		if(activepids[i] != -1){
			// KILL!!!!! DESTROY!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			kill(activepids[i], SIGINT);
		}
	}	
}



/* addToActivePidList
 *
 * Function that adds a number to the active pid list whereever 
 * theres an open spot
 *
 * @ param: childPid: pid to add
 * */
void addToActivePidList(int childPid){
	for(int i=0; i<CHILD_PROCESS_CAP; ++i){
		if(childPid == 0) break;
		if(activepids[i] == -1){
			activepids[i] = childPid;
			num_active_processes++;
			break;
		}
	}

}



/* printChildStatus()
 *
 * Function that prints the previously recieved exit value
 *  or signal from a child process.
 * */
void printChildStatus(void){
	/* Citation for printing out the child exit value
	 * or the child signal.
	 * date: 4/28/2022
	 * Adapted from code given in the lecture
	 * Url: https://canvas.oregonstate.edu/courses/1870063/pages/exploration-process-api-monitoring-child-processes?module_item_id=22026548
	 * */
	if(WIFEXITED(childStatus)){
		// if child was exited
		fprintf(stdout,"exit value  %d\n", WEXITSTATUS(childStatus));
		fflush(stdout);
	}else if(WIFSIGNALED(&childStatus)){
		fprintf(stdout, "signal value %d\n", WTERMSIG(childStatus));
		fflush(stdout);
	}

}



void printUserArgs(UserArgs *Args){
	for(int i=0; i<Args->amount_args; ++i){
		fprintf(stdout, "Arg: |%s|\n", Args->args[i]);
	}
}


void setSignals(){
	// Signal handlers!
 	// Signal handler for when a child finishes:
	//struct sigaction s_child;
	
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

	/*
	struct sigaction sa_z;

	sigemptyset(&sa_z.sa_mask);
	sa_z.sa_flags = SA_RESTART; // flag to make reentry not interrupt user command input
	sa_z.sa_handler = sig_handlerz;
	if(sigaction(SIGTSTP, &sa_z, NULL) == -1){
		fprintf(stdout, "error sig_z\n");
		fflush(stdout);
		exit(1);
	}*/

}



void checkBackgroundProcesses(void){
	/* Citation for sync killing of background / zombie processes
	* date: 5/1/2022
	* Adapted from provided code for synchronous killing of background children
	* from the textbook
	* Book: Linux Programing Interface
	* Author: Michael Kerrisk
	* Page: 543
	 */
	// since we want to keep track of active child processes for the clean up at the end
	// we need to loop through and check each active child, if its finished we removed it
	// so that if we exit while a process is active we can kill it before we quit.
	int status = -5;
	for(int i=0; i<CHILD_PROCESS_CAP; ++i){
		if(activepids[i]>0){
			pid_t doneCheck = waitpid(activepids[i], &status, WNOHANG);
			if(doneCheck > 0){
				// remove child from background process arr
				activepids[i] = 0;
				num_active_processes--;
				childPid = doneCheck;
				childStatus = status;
				// just like the lecture
				if(WIFEXITED(childStatus)){
					fprintf(stdout, "background process %d exited with status %d.\n", doneCheck, WEXITSTATUS(status));
				}else{
					fprintf(stdout, "background process %d terminated with signal %d.\n", doneCheck, WTERMSIG(status));
				}
				fflush(stdout);
			}
		}
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
	if(Args->outfile[0] != '\0'){
		/* Citation for use of dup2 to read in stdin output into infile
		 * Author: Michael Kerrisk
		 * Book: The Linux Programming Interface
		 * Chapter: 27.4 - File Descriptors and exec()
		 * Page: 576
		 * Date: 4/27/2022
		 * Adapted frome example code on use of dup2
		 * */
		// open in file, get contents from execvp, put it in infile.
		int fd = open(Args->outfile, O_WRONLY | O_CREAT | O_TRUNC, 00700);
		if(fd == -1){
			// error
			fprintf(stderr, "Cannot open %s for output.\n", Args->outfile);
			fflush(stderr);
			return 1;
		}
		fflush(stdout);
		if(fd!=STDOUT_FILENO){
			dup2(fd, STDOUT_FILENO);
		}
		close(fd);
	}

	if(Args->infile[0] != '\0'){
		/* Citation for use of dup2 to read in stdin output into infile
 		* Author: Michael Kerrisk
 		* Book: The Linux Programming Interface
 		* Chapter: 27.4 - File Descriptors and exec()
 		* Page: 576
 		* Date: 4/27/2022
 		* Adapted frome example code on use of dup2
 		* */

		// if there is an input file
		int fd2 = open(Args->infile, O_RDONLY);
		if(fd2 == -1){
			// error
			fprintf(stderr, "Cannot open %s for input.\n", Args->infile);
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
		int fd3 = open("/dev/null", O_WRONLY);
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
		chdir(toGoTo);
	}else if(toGoTo){
		if(chdir(toGoTo) == -1){
			// error message but its not enough to exit
			fprintf(stderr, "Error with cd: dir does not exist\n");
			fflush(stderr);
			return;
	
		}
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
			buffer[i-1] == '<' ? f = 1: 1;
			buffer[i-1] == '>' ? f = 2: 1;
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
			//get buffer until " "
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
