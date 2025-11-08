#include "shell.h"
#include <readline/history.h>
#include <ctype.h>

/* forward parse_segments() declared inside shell.c; declare here */
int parse_segments(const char *cmdline,
                   char ****cmds_argv_out,
                   char ***infiles_out,
                   char ***outfiles_out);

int main(void) {
    char *cmdline;
    init_history();
    init_readline();
    init_jobs_table(); /* ensure job table cleared; implemented in jobs.c */

    while (1) {
        /* reap any finished background jobs before showing prompt */
        reap_background_jobs();

        cmdline = read_cmd(PROMPT, stdin);
        if (cmdline == NULL) break; /* EOF / Ctrl-D */

        /* top-level chaining: split on ';' */
        char *line_copy = strdup(cmdline);
        char *saveptr = NULL;
        char *segment = strtok_r(line_copy, ";", &saveptr);
        while (segment) {
            /* trim leading/trailing whitespace */
            char *s = segment;
            while (*s && isspace((unsigned char)*s)) s++;
            char *end = s + strlen(s) - 1;
            while (end >= s && isspace((unsigned char)*end)) { *end = '\0'; end--; }
            if (*s == '\0') { segment = strtok_r(NULL, ";", &saveptr); continue; }

            /* detect background '&' at end */
            int background = 0;
            int len = strlen(s);
            if (len > 0 && s[len-1] == '&') {
                background = 1;
                s[len-1] = '\0';
                /* trim trailing spaces again */
                int newlen = strlen(s);
                while (newlen > 0 && isspace((unsigned char)s[newlen-1])) { s[newlen-1] = '\0'; newlen--; }
            }

            /* parse into pipeline segments (handles |, <, >) */
            char ***cmds_argv = NULL;
            char **infiles = NULL;
            char **outfiles = NULL;
            int ncmds = parse_segments(s, &cmds_argv, &infiles, &outfiles);
            if (ncmds <= 0) { segment = strtok_r(NULL, ";", &saveptr); continue; }

            /* add to histories */
            add_history_cmd(s);
            if (s[0] != '\0') add_history(s);

            /* If single command and it's a builtin -> run builtin in parent (unless background) */
            if (ncmds == 1 && cmds_argv[0] && cmds_argv[0][0]) {
                /* check builtin names */
                if (strcmp(cmds_argv[0][0], "jobs") == 0) {
                    /* jobs builtin: handle directly */
                    print_jobs();
                    /* free memory and continue */
                    for (int i = 0; cmds_argv[0][i]; ++i) free(cmds_argv[0][i]);
                    free(cmds_argv[0]);
                    free(cmds_argv); if (infiles) free(infiles); if (outfiles) free(outfiles);
                    segment = strtok_r(NULL, ";", &saveptr);
                    continue;
                }

                /* other builtins handled via handle_builtin() */
                if (handle_builtin(cmds_argv[0])) {
                    /* if builtin executed and not background, nothing else required */
                    /* If builtin executed but background flag was set, we need to run it in a child and track job */
                    if (background) {
                        pid_t pid = fork();
                        if (pid == 0) {
                            /* child: run the builtin and exit */
                            handle_builtin(cmds_argv[0]);
                            exit(0);
                        } else if (pid > 0) {
                            add_job(pid, s);
                        } else {
                            perror("fork");
                        }
                    }
                    for (int i = 0; cmds_argv[0][i]; ++i) free(cmds_argv[0][i]);
                    free(cmds_argv[0]);
                    free(cmds_argv); if (infiles) free(infiles); if (outfiles) free(outfiles);
                    segment = strtok_r(NULL, ";", &saveptr);
                    continue;
                }
            }

            /* Not a builtin or pipeline: execute (background respected) */
            execute_pipeline(cmds_argv, infiles, outfiles, ncmds, background, s);

            /* free allocated structures */
            for (int i = 0; i < ncmds; ++i) {
                if (cmds_argv[i]) {
                    for (int j = 0; cmds_argv[i][j] != NULL; ++j) free(cmds_argv[i][j]);
                    free(cmds_argv[i]);
                }
                if (infiles && infiles[i]) free(infiles[i]);
                if (outfiles && outfiles[i]) free(outfiles[i]);
            }
            free(cmds_argv); if (infiles) free(infiles); if (outfiles) free(outfiles);

            segment = strtok_r(NULL, ";", &saveptr);
        } /* end each segment */

        free(line_copy);
        free(cmdline);
    } /* main loop */

    free_history();
    printf("\nShell exited.\n");
    return 0;
}
