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



/* CONSTANTS */
#define  MAX_LINE_LENGTH  2048
#define MAX_ARG_NUM  512


struct userArgs{
	char * args[MAX_ARG_NUM]; // contains the args
	char infile[256]; // contains the in file supplied by the user
	char outfile[256]; // contains the outfile supplied by the user
	int background; // & symbol at end == execute in background, so background = 1. Else background = 0
	int amount_args; // For deallocation
};

typedef struct userArgs UserArgs;


/* FUNCTION PROTOTPES */
char *getFullUserInput(void);
int getArgsFromInput(char*, char**);
void userArgsCleanUp(char**, int);
void cdAndUpdatePWD(char*);
/* struct specific function prototype*/
void addArgsToStruct(char **, UserArgs *Args, int);
void displayArgs(UserArgs *Args);



int main(){
	fprintf(stdout, "pid: %d\n", getpid());
	fflush(stdout);


	// init child stuff	
	int childStatus = 0;
	//int childPid;
	pid_t childPid = -5;	
	
	/* TODO: GET ECHO WORKING BEFORE TRYING TO RUN THE TEST SCRIPT*/

	do{	
		// get user input from stdin
		char * userInput = getFullUserInput();
		// clean user input in to args	
		char *user_args[MAX_ARG_NUM];
	
		int amount = getArgsFromInput(userInput, user_args);

		UserArgs Args;
	
		// add the user args to the struct
		addArgsToStruct(user_args, &Args, amount);		
	
		// testing display
		//displayArgs(&Args);

		userArgsCleanUp(user_args, amount);
		free(userInput);

		// check first arg for exit, cd, status in a chain
		if(strcmp(Args.args[0], "exit") == 0 || strcmp(Args.args[0], "exit\n") == 0){
			// deal with handling processes ending
			int z;
			do{
				z = wait(NULL);
				fprintf(stdout, "Freed: %d\n", z);
				fflush(stdout);
			}while(z != -1);		
	
			// memory clean up before returning
			//userArgsCleanUp(Args.args, Args.amount_args);

			for(int i=0; i < Args.amount_args; ++i) free(Args.args[i]);
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
			fprintf(stdout,"Status: %d\n", childStatus);
			fflush(stdout);
		}else{
			// handle other args here
			//fprintf(stdout, "other args\n");
			//fflush(stdout);
			childPid = fork();
			
			// TODO: REMOVE LATER
			// testing: automatically kill process after 10 min
			alarm(600);		
	
			switch(childPid){
				case -1:
					fprintf(stderr, "Failed to fork.\n");
					fflush(stderr);	
					for(int i=0; i < Args.amount_args; ++i) free(Args.args[i]);

					exit(1);
					break;
				case 0:
					// child code
					//fprintf(stdout,"Child successfully spawned\n");
					//fflush(stdout);
						
					if(Args.infile[0] != '\0'){
						/* Citation for use of dup2 to read in stdin output into infile
 						 * Author: Michael Kerrisk
 						 * Book: The Linus Programming Interface
 						 * Chapter: 27.4 - File Descriptors and exec()
 						 * Page: 576
 						 * Date: 4/27/2022
 						 * Adapted frome example code on use of dup2
 						 * */
						// open in file, get contents from execvp, put it in infile.
						int fd = open(Args.infile, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
						if(fd == -1){
							// error
							fprintf(stderr, "Cannot open %s for input\n", Args.infile);
							fflush(stderr);
						}
						fflush(stdout);
						if(fd!=STDOUT_FILENO){
							dup2(fd, STDOUT_FILENO);
						}
						close(fd);
					}

					if(Args.outfile[0] != '\0'){
						/* Citation for use of dup2 to read in stdin output into infile
 						* Author: Michael Kerrisk
 						* Book: The Linus Programming Interface
 						* Chapter: 27.4 - File Descriptors and exec()
 						* Page: 576
 						* Date: 4/27/2022
 						* Adapted frome example code on use of dup2
 						* */

						// if there is an outfile
						//fprintf("outfile\n");
						//fflush(stdout);
					}

					
					// execute command	
					execvp(Args.args[0], Args.args);
					
					fprintf(stderr, "Error with cmd\n");
					fflush(stderr);
					// mem clean up
					for(int i=0; i < Args.amount_args; ++i) free(Args.args[i]);

					exit(2);
					break;
				default:
				
					fflush(stdout);	
					// wait for child to come back
					childPid = waitpid(childPid, &childStatus, 0);
					
					if(WIFEXITED(childStatus)){
						fprintf(stdout, "Child %d exited normally with status %zu\n", childPid, childStatus);
						fflush(stdout);
					}else{
						fprintf(stdout,"Child %d exited abnormally due to signal %zu\n", childPid, childStatus);
						fflush(stdout);
					}
					break;
			}
				
			
	
		}

		// trash collection before loop continues
		for(int i=0; i < Args.amount_args; ++i) free(Args.args[i]);

		//if(Args.infile){
		//	free(Args.infile);
		//}
		//if(Args.outfile){
		//	free(Args.outfile);
		//}
	}while(1);	
	return 0;
}



/* Function that takes arr of arrs user_args and adds it to the struct
 *
 * @param: user_args, raw 2d arr of args
 * */
void addArgsToStruct(char ** user_args, UserArgs *Args, int amount){

	//for(int i=0; i<Args->amount_args; ++i) Args->args[i] = NULL;
	Args->infile[0] = '\0';
	Args->outfile[0] = '\0';
	Args->background = 0;
	Args->amount_args = 0;

	for(size_t i = 0; i<amount; ++i){
		// malloc
		if(user_args[i][0] == '<' && user_args[i][1] == ' ' ){
			// outfile
			strncpy(Args->outfile, user_args[i]+2, strlen(user_args[i])-2);
			Args->outfile[strlen(user_args[i])-2] = '\0';
		}else if(user_args[i][0] == '>' && user_args[i][1] == ' '){
			// infile
			strncpy(Args->infile, user_args[i]+2, strlen(user_args[i])-2);
			Args->infile[strlen(user_args[i])-2] = '\0';
		}else if(strcmp(user_args[i], "&") != 0){
			// add it
			Args->args[Args->amount_args] = malloc(strlen(user_args[i])+1);
			strncpy(Args->args[Args->amount_args], user_args[i], strlen(user_args[i]));
			Args->args[Args->amount_args][strlen(user_args[i])] = '\0';
			Args->amount_args++;
		}
	}

	if(strcmp(user_args[amount-1], "&") == 0){
		// set background
		Args->background = 1;
	}
	Args->args[Args->amount_args] = NULL;
	Args->args[Args->amount_args] = '\0';
	return;
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




/* Function that cleans up a dynamically allocated 2d arr
 *
 * @param user_args: arr to free
 * @param amount: number of elements to free
 * */
void userArgsCleanUp(char *user_args[], int amount){
	for(int i=0; i < amount; ++i){
			if(user_args[i] == NULL){
				break;
			}			
			free(user_args[i]);
		}
	return;

}



/* getArgsFromInput(char*)
 * Function that splits the user input and get the args from the input
 *
 * @param: to_split: string array of the user input to split
 * return: split arr of user args
 * */
int getArgsFromInput(char *to_split, char *user_args[]){

	/* Citation for string tokenization:
 	 * Date: 4/22/2022 
 	 * Adapted from:
 	 * Source URL: https://stackoverflow.com/a/4160297
 	 * */
	char *token = strtok(to_split, " ");
	char *last = token;
	if(token == NULL){
		return 0;
	}
	int i = 0;
	while(token != NULL){
		// add each item to array of arrays
		if(strcmp(token, "&") !=0 && strcmp(token, "<") != 0 && strcmp(token, ">") != 0 && strcmp(last, "<") != 0 && strcmp(last, ">") !=0){
			user_args[i] = malloc(strlen(token)+1);
			strcpy(user_args[i], token);
			user_args[i][strlen(token)] = '\0';
			i++;
		}else if((strcmp(last, "<") == 0 || strcmp(last, ">") == 0) && last != token){
			// create a temp that is the size of the current token + last + 1 for space + 1 for '\0'
			char* temp = malloc(strlen(token)+strlen(last)+2);
			strcpy(temp, last);
			temp[strlen(token)+strlen(last)+1] = '\0';
			// last + space + token + '\0' = < [token]\0
			strcat(temp, " ");
			strcat(temp, token);
			user_args[i] = malloc(strlen(temp)+1);
			strcpy(user_args[i], temp);
			
			// add this to the args
			user_args[i][strlen(temp)] = '\0';
			i++;
			last = temp;
			free(temp);
		}		
		
		last = token;
		// advance the token
		token = strtok(NULL, " ");
	}
	if(strcmp(last, "&") == 0){
		// add it iff last val
		user_args[i] = malloc(strlen(last)+1);
		strcpy(user_args[i], last);
		user_args[i][strlen(last)] = '\0';
		i++;
	}
	user_args[i] = '\0';

	// return size
	return i;

}



/* getFullUserInput(void)
 * Function that reads the user input from the command line,
 * and returns it to main.
 *
 * @params: none
 * returns char * user input pointer.
 * */
char *getFullUserInput(void){
	// init vars
	char buffer[MAX_LINE_LENGTH+1]={1};
	int cmd_size = 0;
		
	// read in user input
	do{
		fprintf(stdout,": "); // cmd line start
		fflush(stdout);
		fgets(buffer, MAX_LINE_LENGTH, stdin);
		buffer[strlen(buffer)] = '\0';	
		if(buffer[strlen(buffer)-1]=='\n'){
			buffer[strlen(buffer)-1] = '\0';
		}
	}while(strlen(buffer) < 2 || buffer[0] == '#');
	
	cmd_size = strlen(buffer);

	// dynamically create the array.
	char *cmd = malloc(cmd_size+1);
	strncpy(cmd, buffer, cmd_size);

	// make sure that the null terminator is properly added.
	cmd[cmd_size] = '\0';

	return cmd;
}
