#include "shell.h"
#include <readline/history.h>


int parse_segments(const char *cmdline,
                   char ****cmds_argv_out,
                   char ***infiles_out,
                   char ***outfiles_out);

int main(void) {
    char *cmdline;
    init_history();
    init_readline();

    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {
        char *p = cmdline;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0') { free(cmdline); continue; }

        if (p[0] == '!') {
            int n = atoi(p + 1);
            if (n <= 0) { fprintf(stderr, "Invalid history reference: %s\n", p); free(cmdline); continue; }
            char *rec = get_history_cmd_by_number(n);
            if (!rec) { fprintf(stderr, "No such history entry: %d\n", n); free(cmdline); continue; }
            free(cmdline); cmdline = rec; p = cmdline;
        }


        char ***cmds_argv = NULL;
        char **infiles = NULL;
        char **outfiles = NULL;
        int ncmds = parse_segments(cmdline, &cmds_argv, &infiles, &outfiles);
        if (ncmds <= 0) { free(cmdline); continue; }


        add_history_cmd(cmdline);
        if (cmdline[0] != '\0') add_history(cmdline);


        if (ncmds == 1 && cmds_argv[0] && handle_builtin(cmds_argv[0])) {
            /* free memory for cmds_argv/infiles/outfiles */
            if (cmds_argv[0]) { for (int i=0; cmds_argv[0][i]; ++i) free(cmds_argv[0][i]); free(cmds_argv[0]); }
            free(cmds_argv); if (infiles) free(infiles); if (outfiles) free(outfiles);
            free(cmdline);
            continue;
        }


        execute_pipeline(cmds_argv, infiles, outfiles, ncmds);

        /* free allocated structures */
        for (int i = 0; i < ncmds; ++i) {
            if (cmds_argv[i]) {
                for (int j = 0; cmds_argv[i][j] != NULL; ++j) free(cmds_argv[i][j]);
                free(cmds_argv[i]);
            }
            if (infiles && infiles[i]) free(infiles[i]);
            if (outfiles && outfiles[i]) free(outfiles[i]);
        }
        free(cmds_argv);
        if (infiles) free(infiles);
        if (outfiles) free(outfiles);

        free(cmdline);
    }

    free_history();
    printf("\nShell exited.\n");
    return 0;
}
