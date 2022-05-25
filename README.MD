# SmallSH - Small Shell
**Name**: Alyssa Comstock

**Class**: CS344 - Operaing Systems

**Homework**: Program 3 SmallSH Portfolio assignment

**Date**: 5/9/2022

**Programming Language**: C


All code adapted / taken from lectures and textbook examples cited in code.


## Description of Assignment:
Write a small shell in C. This shell must implement some feaures found in other shells such as bash. Your program must:
1. Provide a prompt for running commands.
2. Handle blank lines and comments, which are lines beginning with the `#` character.
3. Provide exansion for the variable `$$`.
4. Execute 3 commands `exit`, `cd`, and `status` via code built in to the shell.
5. Execute other commands by creating new processes using a function from the `exec` family of functions.
6. Support input and output redirection.
7. Support running commands in foreground and background processes.
8. Implement custom handlers for 2 signals, `SIGINT` and `SIGTSTP`.
