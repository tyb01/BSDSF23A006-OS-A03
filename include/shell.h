#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_LEN 512
#define MAXARGS 10
#define ARGLEN 30
#define PROMPT "FCIT> "

// Function prototypes
char* read_cmd(char* prompt, FILE* fp);
char** tokenize(char* cmdline);
int execute(char** arglist);
int handle_builtin(char **arglist);

// history config
#define HISTORY_SIZE 50

/* history API */
void init_history(void);
void free_history(void);
void add_history_cmd(const char *cmd);
void print_history(void);
char *get_history_cmd_by_number(int n);


#endif // SHELL_H
