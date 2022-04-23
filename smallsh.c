#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* FUNCTION PROTOTPES */
char *getFullUserInput(void);
char *getArgsFromInput(char*);

/* CONSTANTS */
const intmax_t MAX_LINE_LENGTH = 2048;
const intmax_t MAX_ARG_NUM = 512;

struct userArgs{
	char *args[MAX_ARG_NUM];
	char *in_file; // contains the in file supplied by the user
	char *out_file; // contains the outfile supplied by the user
	int background; // & symbol at end == execute in background, so background = 1. Else background = 0
}

int main(){
	
	// get user input from stdin
	char * userInput = getFullUserInput();
	// clean user input in to args	
	

	// trash collection
	free(userInput);	
	return 0;

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
	char buffer[256]={1};
	int i = 0;
	int cmd_size = 0;

	// read in user input
	printf(": "); // cmd line start
	scanf("%s",buffer);
	buffer[strlen(buffer)] = '\0';	

	cmd_size = strlen(buffer);

	// dynamically create the array.
	char *cmd = malloc(cmd_size+1);
	strncpy(cmd, buffer, cmd_size);

	// make sure that the null terminator is properly added.
	cmd[cmd_size] = '\0';

	return cmd;
}
