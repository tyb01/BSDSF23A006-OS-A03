#include "shell.h"
#include <readline/history.h>

int main(void) {
    char *cmdline, **arglist;
    init_history();
    init_readline();

    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {
        char *p = cmdline; while (*p==' '||*p=='\t') p++;
        if (*p=='\0') { free(cmdline); continue; }

        if (p[0]=='!') {
            int n = atoi(p+1);
            char *rec = get_history_cmd_by_number(n);
            if (!rec) { fprintf(stderr,"No such history: %d\n", n); free(cmdline); continue; }
            free(cmdline); cmdline = rec; p = cmdline;
        }

        if ((arglist = tokenize(cmdline))) {
            add_history_cmd(cmdline);
            if (*cmdline) add_history(cmdline);  // For arrow-key recall

            if (!handle_builtin(arglist))
                execute(arglist);

            for (int i=0; arglist[i]; i++) free(arglist[i]);
            free(arglist);
        }
        free(cmdline);
    }

    free_history();
    printf("\nShell exited.\n");
    return 0;
}
