/* Name: Alyssa Comstock
 * Class: CS344 - Operating Systems
 * Assignment: Portfolio Project - SmallSH
 * Date: 4/29/2022
 * Descrption: Header file that contains the struct definition, contants,
 * function prototypes, and global variables for the main smallsh.c file
 * */

/* CONSTANTS */
#define  MAX_LINE_LENGTH  2048
#define MAX_ARG_NUM  512

/* STRUCT DEFINITION*/
struct userArgs{
	char * args[MAX_ARG_NUM]; // contains the args
	char infile[256]; // contains the in file supplied by the user
	char outfile[256]; // contains the outfile supplied by the user
	int background; // & symbol at end == execute in background, so background = 1. Else background = 0
	int amount_args; // For deallocation
};

typedef struct userArgs UserArgs;


/* FUNCTION PROTOTPES */
int getFullUserInput(UserArgs *Args);
void cdAndUpdatePWD(char*);
int handleRedirection(UserArgs *Args);


/* STRUCT SPECIFIC PROTOTYPES*/
void displayArgs(UserArgs *Args); // to remove
void clearArgs(UserArgs *Args); // set everything to 0 or null
void dealloArgs(UserArgs *Args); // deallocates the malloc'd memory in args struct

/* SIGNAL HANDLERS */
void sig_handler(int sig);
void sig_handler2(int sig);
void zombie_handler();

