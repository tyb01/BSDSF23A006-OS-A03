#include "shell.h"

int main(void) {
    char *cmdline;
    char **arglist;

    init_history();

    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {
        char *p = cmdline;
        while (*p == ' ' || *p == '\t') p++;

        if (*p == '\0') { // empty line
            free(cmdline);
            continue;
        }

        if (p[0] == '!') {
            int n = atoi(p + 1);
            if (n <= 0) {
                fprintf(stderr, "Invalid history reference: %s\n", p);
                free(cmdline);
                continue;
            }
            char *recalled = get_history_cmd_by_number(n);
            if (recalled == NULL) {
                fprintf(stderr, "No such history entry: %d\n", n);
                free(cmdline);
                continue;
            }

            free(cmdline);
            cmdline = recalled;
            p = cmdline;
        }


        if ((arglist = tokenize(cmdline)) != NULL) {


            add_history_cmd(cmdline);


            if (handle_builtin(arglist)) {

                for (int i = 0; arglist[i] != NULL; i++) free(arglist[i]);
                free(arglist);
                free(cmdline);
                continue;
            }


            execute(arglist);

            // Free arglist memory
            for (int i = 0; arglist[i] != NULL; i++) free(arglist[i]);
            free(arglist);
        }

        free(cmdline);
    }

    free_history();
    printf("\nShell exited.\n");
    return 0;
}
