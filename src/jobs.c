#include "shell.h"
#include <sys/wait.h>

#define MAX_JOBS 128

typedef struct {
    pid_t pid;
    char *cmd;
} job_t;

static job_t jobs[MAX_JOBS];

void init_jobs_table(void) {
    for (int i = 0; i < MAX_JOBS; ++i) { jobs[i].pid = 0; jobs[i].cmd = NULL; }
}

void add_job(pid_t pid, const char *cmdline) {
    if (pid <= 0) return;
    for (int i = 0; i < MAX_JOBS; ++i) {
        if (jobs[i].pid == 0) {
            jobs[i].pid = pid;
            jobs[i].cmd = strdup(cmdline ? cmdline : "");
            printf("[+] Background job started: PID %d\n", pid);
            return;
        }
    }
    fprintf(stderr, "jobs: job table full, cannot add PID %d\n", pid);
}

void remove_job(pid_t pid) {
    for (int i = 0; i < MAX_JOBS; ++i) {
        if (jobs[i].pid == pid) {
            free(jobs[i].cmd);
            jobs[i].cmd = NULL;
            jobs[i].pid = 0;
            return;
        }
    }
}

void print_jobs(void) {
    int found = 0;
    for (int i = 0; i < MAX_JOBS; ++i) {
        if (jobs[i].pid != 0) {
            printf("%d\t%s\n", jobs[i].pid, jobs[i].cmd ? jobs[i].cmd : "");
            found = 1;
        }
    }
    if (!found) printf("No background jobs.\n");
}

void reap_background_jobs(void) {
    int status;
    pid_t pid;
    /* Non-blocking loop to reap all finished children */
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        /* Print notification */
        if (WIFEXITED(status)) {
            int code = WEXITSTATUS(status);
            printf("[+] Job %d finished (exit %d)\n", pid, code);
        } else if (WIFSIGNALED(status)) {
            int sig = WTERMSIG(status);
            printf("[+] Job %d terminated by signal %d\n", pid, sig);
        } else {
            printf("[+] Job %d ended\n", pid);
        }
        remove_job(pid);
    }
    /* if pid == 0 => no children have exited; if pid == -1 and errno==ECHILD => nothing to wait for */
}
