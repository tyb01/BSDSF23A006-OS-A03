#include "shell.h"

#include <readline/readline.h>
#include <readline/history.h>
#include <strings.h>
#include <ctype.h>


char* read_cmd(char* prompt, FILE* fp) {
    if (fp == stdin && isatty(fileno(stdin))) {
        char *line = readline(prompt);
        if (!line) return NULL; /* Ctrl-D */
        return line;            /* caller must free */
    }

    printf("%s", prompt);
    char *buf = malloc(MAX_LEN);
    int c, pos = 0;
    while ((c = getc(fp)) != EOF && c != '\n') {
        if (pos < MAX_LEN - 1) buf[pos++] = c;
    }
    if (c == EOF && pos == 0) { free(buf); return NULL; }
    buf[pos] = '\0';
    return buf;
}


void init_readline(void) {
    rl_bind_key('\t', rl_complete);
}


char** tokenize(char* cmdline) {
    if (cmdline == NULL) return NULL;
    /* trim trailing newline if present */
    size_t L = strlen(cmdline);
    if (L > 0 && cmdline[L-1] == '\n') cmdline[L-1] = '\0';

    /* empty check */
    char *tmp = cmdline;
    while (*tmp == ' ' || *tmp == '\t') tmp++;
    if (*tmp == '\0') return NULL;

    char** arglist = malloc(sizeof(char*) * (MAXARGS + 1));
    for (int i = 0; i < MAXARGS + 1; i++) {
        arglist[i] = malloc(ARGLEN);
        bzero(arglist[i], ARGLEN);
    }

    char* cp = cmdline;
    char* start;
    int argnum = 0;

    while (*cp != '\0' && argnum < MAXARGS) {
        while (*cp == ' ' || *cp == '\t') cp++; // skip
        if (*cp == '\0') break;
        start = cp;
        while (*cp && *cp != ' ' && *cp != '\t') cp++;
        int len = cp - start;
        if (len >= ARGLEN) len = ARGLEN - 1;
        strncpy(arglist[argnum], start, len);
        arglist[argnum][len] = '\0';
        argnum++;
    }

    if (argnum == 0) {
        for (int i = 0; i < MAXARGS + 1; i++) free(arglist[i]);
        free(arglist);
        return NULL;
    }
    arglist[argnum] = NULL;
    return arglist;
}


extern void print_history(void);

int handle_builtin(char **arglist) {
    if (arglist == NULL || arglist[0] == NULL) return 0;
    if (strcmp(arglist[0], "exit") == 0) { free_history(); exit(0); }
    if (strcmp(arglist[0], "cd") == 0) {
        if (arglist[1] == NULL) fprintf(stderr, "cd: missing argument\n");
        else if (chdir(arglist[1]) != 0) perror("cd");
        return 1;
    }
    if (strcmp(arglist[0], "help") == 0) {
        printf("Built-in commands: cd, exit, help, history, jobs\n"); return 1;
    }
    if (strcmp(arglist[0], "history") == 0) { print_history(); return 1; }
    if (strcmp(arglist[0], "jobs") == 0) { printf("jobs: not implemented yet\n"); return 1; }
    return 0;
}

int parse_segments(const char *cmdline,
                   char ****cmds_argv_out,
                   char ***infiles_out,
                   char ***outfiles_out)
{
    if (!cmdline) return 0;

    /* make a modifiable copy */
    char *copy = strdup(cmdline);
    char *saveptr = NULL;
    int maxseg = 16;
    char **segments = malloc(sizeof(char*) * maxseg);
    int n = 0;

    char *tok = strtok_r(copy, "|", &saveptr);
    while (tok) {
        /* trim leading/trailing spaces */
        while (*tok == ' ' || *tok == '\t') tok++;
        char *end = tok + strlen(tok) - 1;
        while (end > tok && (*end == ' ' || *end == '\t')) { *end = '\0'; end--; }

        if (n >= maxseg) {
            maxseg *= 2;
            segments = realloc(segments, sizeof(char*) * maxseg);
        }
        segments[n++] = strdup(tok);
        tok = strtok_r(NULL, "|", &saveptr);
    }
    free(copy);

    if (n == 0) { free(segments); return 0; }


    char ***cmds_argv = malloc(sizeof(char**) * n);
    char **infiles = malloc(sizeof(char*) * n);
    char **outfiles = malloc(sizeof(char*) * n);
    for (int i = 0; i < n; ++i) { infiles[i] = NULL; outfiles[i] = NULL; cmds_argv[i] = NULL; }


    for (int i = 0; i < n; ++i) {
        char *seg = segments[i];

        char *buf = strdup(seg);
        char *p = buf;
        char *words[ MAXARGS + 10 ];
        int w = 0;


        while (*p) {
            while (*p == ' ' || *p == '\t') p++;
            if (*p == '\0') break;
            if (*p == '<' || *p == '>') {
                char s[2] = {*p, '\0'}; words[w++] = strdup(s); p++; continue;
            }
            char *start = p;
            while (*p && *p != ' ' && *p != '\t' && *p != '<' && *p != '>') p++;
            int len = p - start;
            char *wstr = malloc(len+1);
            strncpy(wstr, start, len); wstr[len] = '\0';
            words[w++] = wstr;
        }


        char *cmdbuf = malloc(MAX_LEN);
        cmdbuf[0] = '\0';
        int cmd_written = 0;
        for (int j = 0; j < w; ++j) {
            if (strcmp(words[j], "<") == 0) {
                if (j+1 < w) {
                    infiles[i] = strdup(words[j+1]); j++;
                }
            } else if (strcmp(words[j], ">") == 0) {
                if (j+1 < w) {
                    outfiles[i] = strdup(words[j+1]); j++;
                }
            } else {

                if (cmd_written) strncat(cmdbuf, " ", MAX_LEN - strlen(cmdbuf) - 1);
                strncat(cmdbuf, words[j], MAX_LEN - strlen(cmdbuf) - 1);
                cmd_written = 1;
            }
        }


        if (cmd_written) {
            cmds_argv[i] = tokenize(cmdbuf);
        } else {
            cmds_argv[i] = NULL;
        }


        for (int j = 0; j < w; ++j) free(words[j]);
        free(buf);
        free(cmdbuf);
        free(segments[i]);
    }
    free(segments);

    *cmds_argv_out = cmds_argv;
    *infiles_out = infiles;
    *outfiles_out = outfiles;
    return n;
}
