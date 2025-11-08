#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>

#define MAX_LEN 512
#define MAXARGS 10
#define ARGLEN 30
#define PROMPT "FCIT> "

/* Basic APIs */
char* read_cmd(char* prompt, FILE* fp);
char** tokenize(char* cmdline);
int handle_builtin(char **arglist);

/* Readline */
void init_readline(void);


int execute_pipeline(char ***cmds_argv, char **infiles, char **outfiles, int ncmds, int background, const char *cmdline_for_job);

/* Jobs API (background job manager) */
void init_jobs_table(void);
void add_job(pid_t pid, const char *cmdline);
void remove_job(pid_t pid);
void print_jobs(void);
void reap_background_jobs(void);

/* Variables API */
void set_var(const char *name, const char *value);
char *get_var(const char *name); /* returns malloc'd string (caller frees); NULL if not set */
void print_vars(void);
void free_vars(void);
void expand_argv_inplace(char **argv); /* expands $VARNAME in argv array in-place */

/* history config */
#define HISTORY_SIZE 50

/* history API */
void init_history(void);
void free_history(void);
void add_history_cmd(const char *cmd);
void print_history(void);
char *get_history_cmd_by_number(int n);

#endif // SHELL_H
