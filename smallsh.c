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
#include <sys/wait.h>
#include <unistd.h>


/* CONSTANTS */
#define  MAX_LINE_LENGTH  2048
#define MAX_ARG_NUM  512

/*
struct userArgs{
	char ** args; // contains the args
	char *infile; // contains the in file supplied by the user
	char *outfile; // contains the outfile supplied by the user
	int background; // & symbol at end == execute in background, so background = 1. Else background = 0
};

typedef struct userArgs UserArgs;
*/

/* FUNCTION PROTOTPES */
char *getFullUserInput(void);
int getArgsFromInput(char*, char**);
void userArgsCleanUp(char**, int);
void cdAndUpdatePWD(char*);
/* struct specific function prototype*/
//void addArgsToStruct(char **, UserArgs *Args, int);


int main(){
	fprintf(stdout, "pid: %d\n", getpid());
	fflush(stdout);
	do{	
		// get user input from stdin
		char * userInput = getFullUserInput();
		// clean user input in to args	
		char *user_args[MAX_ARG_NUM];
		int amount = getArgsFromInput(userInput, user_args);	
		
		// create the userArgs struct
		//UserArgs Args;
		
		// add the user args to the struct
		//addArgsToStruct(user_args, &Args, amount);		
	
		// check first arg for exit, cd, status in a chain
		if(strcmp(user_args[0], "exit") == 0 || strcmp(user_args[0], "exit\n") == 0){
			// deal with handling processes and
		
			/* TODO:
 			*  - CHECK FOR ZOMBIE PROCESSES AND WAIT FOR THEM TO END
 			* */

			// memory clean up before returning
			free(userInput);
			userArgsCleanUp(user_args, amount);
			// we are done here!
			exit(0);
		}else if(strcmp(user_args[0], "cd") == 0){
			// cd to user defined dir in the command
			fprintf(stdout, "cd to dir: %s\n", user_args[1]);
			fflush(stdout);
			/* TODO: 
 			*   - MAKE SURE THAT ABSOLUTE AND RELATIVE PATHS BOTH WORK
 			*   - MAKE SURE THAT IF THERE ARE NO ARGUMENTS, CHANGE TO WHERE THE HOME ENV VAR IS 
 			*     SPECIFIED
 			*/
			cdAndUpdatePWD(user_args[1]);
				
		}else if(strcmp(user_args[0], "status") == 0 || strcmp(user_args[0], "status\n") == 0){
			fprintf(stdout, "post status\n");
			fflush(stdout);
			// check the status
		}else{
			// handle other args here
			fprintf(stdout, "other args\n");
			fflush(stdout);
		}

		// trash collection before loop continues
		userArgsCleanUp(user_args, amount);
		free(userInput);
	}while(1);	
	return 0;
}


/*
void addArgsToStruct(char ** user_args, UserArgs *Args, int amount){
	fprintf(stdout,"made it!\n%d\n", amount);
	fflush(stdout);
	int amount_args = 0;
	Args->args = malloc(amount + 1);
	for(size_t i = 0; i<amount; ++i){
		// malloc
		if(user_args[i][0] != '>' && user_args[i][0] != '<' && strcmp(user_args[i], "&") != 0){
			// add it
			fprintf(stdout, "adding <3 %s\n", user_args[i]);
			fflush(stdout);
			Args->args[i] = malloc(strlen(user_args[i])+1);
			strcpy(Args->args[i], user_args[i]);
			Args->args[i][strlen(user_args[i])] = '\0';
			amount_args++;
		}else if(user_args[i][0] == '>' ){
			// outfile
			fprintf(stdout, "out file\n");
			fflush(stdout);
			Args->outfile = malloc(strlen(user_args[i])+1);
			strcpy(Args->outfile, user_args[i]);
			Args->outfile[strlen(user_args[i])] = '\0';
		}else if(user_args[i][0] == '<'){
			// infile
			fprintf(stdout, "in file\n");
			fflush(stdout);
			Args->infile = malloc(strlen(user_args[i])+1);
			strcpy(Args->infile, user_args[i]);
			Args->infile[strlen(user_args[i])] = '\0';
		}else if(strcmp(user_args[i], "&") == 0){
			// run in background flag
			fprintf(stdout, "run in background\n");
			fflush(stdout);
		}
	}
	Args->args = realloc(Args->args, amount_args+1);
	Args->args[amount_args] = '\0';
}*/



void cdAndUpdatePWD(char * toGoTo){
	char buffer[256];
	int res = chdir(toGoTo);
	if(res == 0){
		// we successfully changed dir, need to update pwd
		getcwd(buffer, 255);
		fprintf(stdout, "buffer size: %zu\n", strlen(buffer));
		fflush(stdout);
		buffer[strlen(buffer)] = '\0';
		setenv("PWD", buffer, 1);
	}else{
		// error message but its not enough to exit
		fprintf(stderr, "Error with cd: dir does not exist\n");
		fflush(stderr);
	}

	fprintf(stdout,"pwd: %s\npwdenv: %s\n", buffer, getenv("PWD"));
	fflush(stdout);
}




/* Function that cleans up a dynamically allocated 2d arr
 *
 * @param user_args: arr to free
 * @param amount: number of elements to free
 * */
void userArgsCleanUp(char *user_args[], int amount){
	for(int i=0; i < amount; ++i){
			if(user_args[i] == NULL){
				//free(user_args);
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
