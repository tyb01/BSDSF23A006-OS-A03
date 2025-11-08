#include "shell.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <strings.h>

char* read_cmd(char* prompt, FILE* fp) {
    /* Use GNU Readline if stdin is interactive */
    if (fp == stdin && isatty(fileno(stdin))) {
        char *line = readline(prompt);
        if (!line) return NULL;       // Ctrl+D
        return line;                  // malloc'd → caller frees
    }

    /* Non-interactive fallback */
    printf("%s", prompt);
    char *cmd = malloc(MAX_LEN);
    int c, pos = 0;
    while ((c = getc(fp)) != EOF && c != '\n')
        cmd[pos++] = c;
    if (c == EOF && pos == 0) { free(cmd); return NULL; }
    cmd[pos] = '\0';
    return cmd;
}

void init_readline(void) {
    rl_bind_key('\t', rl_complete);   // Enable Tab completion
}

char** tokenize(char* cmdline) {
    if (!cmdline || cmdline[0] == '\0' || cmdline[0] == '\n')
        return NULL;

    char **arglist = malloc(sizeof(char*) * (MAXARGS + 1));
    for (int i = 0; i < MAXARGS + 1; i++) {
        arglist[i] = malloc(ARGLEN);
        bzero(arglist[i], ARGLEN);
    }

    char *cp = cmdline, *start;
    int argnum = 0;
    while (*cp && argnum < MAXARGS) {
        while (*cp == ' ' || *cp == '\t') cp++;
        if (*cp == '\0') break;
        start = cp;
        while (*cp && *cp != ' ' && *cp != '\t') cp++;
        strncpy(arglist[argnum++], start, cp - start);
    }
    if (argnum == 0) { for (int i=0;i<=MAXARGS;i++) free(arglist[i]); free(arglist); return NULL; }
    arglist[argnum] = NULL;
    return arglist;
}

extern void print_history(void);

int handle_builtin(char **arglist) {
    if (!arglist[0]) return 0;
    if (!strcmp(arglist[0], "exit")) { free_history(); exit(0); }
    if (!strcmp(arglist[0], "cd")) {
        if (!arglist[1]) fprintf(stderr,"cd: missing arg\n");
        else if (chdir(arglist[1]) != 0) perror("cd");
        return 1;
    }
    if (!strcmp(arglist[0], "help")) { printf("Built-ins: cd, exit, help, history, jobs\n"); return 1; }
    if (!strcmp(arglist[0], "history")) { print_history(); return 1; }
    if (!strcmp(arglist[0], "jobs")) { printf("jobs: not implemented\n"); return 1; }
    return 0;
}
