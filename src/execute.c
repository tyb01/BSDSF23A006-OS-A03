#include "shell.h"
#include <sys/stat.h>
#include <errno.h>

/* Helper: free argv array */
static void free_argv(char **argv) {
    if (!argv) return;
    for (int i = 0; argv[i] != NULL; ++i) free(argv[i]);
    free(argv);
}

int execute_pipeline(char ***cmds_argv, char **infiles, char **outfiles, int ncmds) {
    if (ncmds <= 0) return -1;
    if (ncmds == 1) {
        char **argv = cmds_argv[0];
        if (!argv || !argv[0]) return 0; /* nothing to run */

        pid_t pid = fork();
        if (pid == -1) { perror("fork"); return -1; }
        if (pid == 0) {
            /* child: handle redirection if any */
            if (infiles && infiles[0]) {
                int fd = open(infiles[0], O_RDONLY);
                if (fd == -1) { perror("open infile"); exit(1); }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
            if (outfiles && outfiles[0]) {
                int fd = open(outfiles[0], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd == -1) { perror("open outfile"); exit(1); }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
            execvp(argv[0], argv);
            perror("execvp"); exit(1);
        } else {
            int status; waitpid(pid, &status, 0);
            return 0;
        }
    }

    /* Multiple commands: set up (ncmds-1) pipes */
    int pipes[ncmds-1][2];
    for (int i = 0; i < ncmds-1; ++i) {
        if (pipe(pipes[i]) == -1) { perror("pipe"); return -1; }
    }

    for (int i = 0; i < ncmds; ++i) {
        pid_t pid = fork();
        if (pid == -1) { perror("fork"); return -1; }
        if (pid == 0) {
            /* child i */
            /* if not first, set stdin from previous pipe read end */
            if (i > 0) {
                dup2(pipes[i-1][0], STDIN_FILENO);
            } else if (infiles && infiles[i]) {
                int fd = open(infiles[i], O_RDONLY);
                if (fd == -1) { perror("open infile"); exit(1); }
                dup2(fd, STDIN_FILENO); close(fd);
            }

            /* if not last, set stdout to this pipe write end */
            if (i < ncmds - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            } else if (outfiles && outfiles[i]) {
                int fd = open(outfiles[i], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd == -1) { perror("open outfile"); exit(1); }
                dup2(fd, STDOUT_FILENO); close(fd);
            }

            /* close all pipe fds in child */
            for (int j = 0; j < ncmds-1; ++j) {
                close(pipes[j][0]); close(pipes[j][1]);
            }

            /* exec */
            char **argv = cmds_argv[i];
            if (!argv || !argv[0]) exit(0);
            execvp(argv[0], argv);
            perror("execvp"); exit(1);
        }
        /* parent continues to spawn other children */
    }

    /* Parent: close all pipe fds */
    for (int i = 0; i < ncmds-1; ++i) { close(pipes[i][0]); close(pipes[i][1]); }

    /* Wait for all children */
    for (int i = 0; i < ncmds; ++i) {
        int status;
        wait(&status);
    }

    return 0;
}
