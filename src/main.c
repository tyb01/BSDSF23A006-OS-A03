#include "shell.h"
#include <readline/history.h>
#include <ctype.h>


int parse_segments(const char *cmdline,
                   char ****cmds_argv_out,
                   char ***infiles_out,
                   char ***outfiles_out);


static void handle_if_block(char *first_line) {

    char *cond = NULL;
    char *p = first_line;

    /* skip leading "if" */
    while (*p && isspace((unsigned char)*p)) p++;
    if (strncmp(p, "if", 2) == 0) p += 2;
    /* trim leading spaces */
    while (*p && isspace((unsigned char)*p)) p++;

    /* p now points to condition (maybe empty or contains 'then') */
    char *then_pos = NULL;
    /* If condition contains "then" at end, split */
    {
        char *tmp = strdup(p);
        char *tpos = strstr(tmp, "then");
        if (tpos) {
            /* condition is content before 'then' */
            *tpos = '\0';
            /* trim trailing spaces */
            char *end = tmp + strlen(tmp) - 1;
            while (end >= tmp && isspace((unsigned char)*end)) { *end = '\0'; end--; }
            cond = strdup(tmp);
        } else {
            if (*p != '\0') cond = strdup(p);
        }
        free(tmp);
    }

    /* If cond empty, read lines until we get a non-empty one (simple support) */
    while ((!cond || cond[0] == '\0')) {
        char *ln = read_cmd("> ", stdin);
        if (!ln) { free(cond); return; }
        /* trim */
        char *t = ln; while (*t && isspace((unsigned char)*t)) t++;
        if (!*t) { free(ln); continue; }
        /* if this line is "then", then continue to next — but cond can't be empty if then appears */
        if (strcmp(t, "then") == 0) { free(ln); break; }
        cond = strdup(t);
        free(ln);
        break;
    }

    /* Now we need to read until we find a 'then' line (if not already consumed) */
    int saw_then = 0;
    if (cond) {
        /* If initial first_line included 'then' we already handled; else check next lines for 'then' */
        /* If cond contains trailing 'then' already removed above. So now read lines until 'then' */
        while (!saw_then) {
            char *ln = read_cmd("> ", stdin);
            if (!ln) { free(cond); return; }
            char *t = ln; while (*t && isspace((unsigned char)*t)) t++;
            /* detect 'then' */
            if (strcmp(t, "then") == 0) { saw_then = 1; free(ln); break; }
            /* If user typed the condition and then 'then' on same line, we earlier split; unlikely here */
            free(ln);
        }
    }

    /* Collect then-block lines until 'else' or 'fi' */
    char **then_lines = NULL;
    int then_count = 0;
    char **else_lines = NULL;
    int else_count = 0;
    int in_else = 0;

    while (1) {
        char *ln = read_cmd("> ", stdin);
        if (!ln) break;
        char *t = ln;
        while (*t && isspace((unsigned char)*t)) t++;
        /* trim trailing spaces/newlines */
        char *end = t + strlen(t) - 1;
        while (end >= t && isspace((unsigned char)*end)) { *end = '\0'; end--; }

        if (strcmp(t, "fi") == 0) { free(ln); break; }
        if (strcmp(t, "else") == 0) { in_else = 1; free(ln); continue; }

        if (!in_else) {
            then_lines = realloc(then_lines, sizeof(char*) * (then_count + 1));
            then_lines[then_count++] = strdup(t);
        } else {
            else_lines = realloc(else_lines, sizeof(char*) * (else_count + 1));
            else_lines[else_count++] = strdup(t);
        }
        free(ln);
    }

    /* Execute the condition command synchronously and get exit status */
    int status = -1;
    if (cond && cond[0] != '\0') {
        char ***cmds_argv = NULL;
        char **infiles = NULL;
        char **outfiles = NULL;
        int ncmds = parse_segments(cond, &cmds_argv, &infiles, &outfiles);
        if (ncmds > 0) {
            status = execute_pipeline(cmds_argv, infiles, outfiles, ncmds, 0, cond);
            /* free parsed structures for condition */
            for (int i = 0; i < ncmds; ++i) {
                if (cmds_argv[i]) {
                    for (int j = 0; cmds_argv[i][j] != NULL; ++j) free(cmds_argv[i][j]);
                    free(cmds_argv[i]);
                }
                if (infiles && infiles[i]) free(infiles[i]);
                if (outfiles && outfiles[i]) free(outfiles[i]);
            }
            free(cmds_argv); if (infiles) free(infiles); if (outfiles) free(outfiles);
        }
    }

    /* Based on status (0 = success) execute then_lines or else_lines */
    char **chosen = (status == 0) ? then_lines : else_lines;
    int chosen_count = (status == 0) ? then_count : else_count;

    for (int i = 0; i < chosen_count; ++i) {
        char *line = chosen[i];
        /* reuse existing flow: parse_segments -> execute_pipeline (foreground) */
        char ***cmds_argv = NULL;
        char **infiles = NULL;
        char **outfiles = NULL;
        int ncmds = parse_segments(line, &cmds_argv, &infiles, &outfiles);
        if (ncmds > 0) {
            /* add to history (keeps behavior consistent) */
            add_history_cmd(line);
            if (line[0] != '\0') add_history(line);
            execute_pipeline(cmds_argv, infiles, outfiles, ncmds, 0, line);
            for (int k = 0; k < ncmds; ++k) {
                if (cmds_argv[k]) {
                    for (int j = 0; cmds_argv[k][j] != NULL; ++j) free(cmds_argv[k][j]);
                    free(cmds_argv[k]);
                }
                if (infiles && infiles[k]) free(infiles[k]);
                if (outfiles && outfiles[k]) free(outfiles[k]);
            }
            free(cmds_argv); if (infiles) free(infiles); if (outfiles) free(outfiles);
        }
    }

    /* cleanup */
    if (cond) free(cond);
    for (int i = 0; i < then_count; ++i) free(then_lines[i]);
    for (int i = 0; i < else_count; ++i) free(else_lines[i]);
    free(then_lines);
    free(else_lines);
}

/* main loop (merges chaining, background handling, and if blocks) */
int main(void) {
    char *cmdline;
    init_history();
    init_readline();
    init_jobs_table();

    while (1) {
        reap_background_jobs();              /* collect finished background jobs */
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

            /* If this is an 'if' block start, handle the entire structure */
            /* detect leading 'if' token (very simple) */
            char tmp[MAX_LEN];
            strncpy(tmp, s, MAX_LEN-1); tmp[MAX_LEN-1]='\0';
            char *tok = tmp;
            while (*tok && isspace((unsigned char)*tok)) tok++;
            if (strncmp(tok, "if", 2) == 0 && (tok[2] == ' ' || tok[2] == '\0' || tok[2] == '\t')) {
                /* handle multiline if-then-else-fi; pass the original segment string */
                handle_if_block(s);
                segment = strtok_r(NULL, ";", &saveptr);
                continue;
            }

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
                if (strcmp(cmds_argv[0][0], "jobs") == 0) {
                    print_jobs();
                    for (int i = 0; cmds_argv[0][i]; ++i) free(cmds_argv[0][i]);
                    free(cmds_argv[0]);
                    free(cmds_argv); if (infiles) free(infiles); if (outfiles) free(outfiles);
                    segment = strtok_r(NULL, ";", &saveptr);
                    continue;
                }
                if (handle_builtin(cmds_argv[0])) {
                    if (background) {
                        pid_t pid = fork();
                        if (pid == 0) { handle_builtin(cmds_argv[0]); exit(0); }
                        else if (pid > 0) add_job(pid, s);
                        else perror("fork");
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
