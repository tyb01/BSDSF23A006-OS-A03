#include "shell.h"

/* Circular buffer for history */
static char *history_buf[HISTORY_SIZE];
static int history_count = 0;   // how many commands currently stored (<= HISTORY_SIZE)
static int history_next = 0;    // index where next command will be written

void init_history(void) {
    for (int i = 0; i < HISTORY_SIZE; i++) history_buf[i] = NULL;
}

void free_history(void) {
    for (int i = 0; i < HISTORY_SIZE; i++) {
        free(history_buf[i]);
        history_buf[i] = NULL;
    }
    history_count = 0;
    history_next = 0;
}

void add_history_cmd(const char *cmd) {
    if (cmd == NULL || *cmd == '\0') return;

    // If overwriting existing entry, free it
    free(history_buf[history_next]);
    history_buf[history_next] = strdup(cmd);

    history_next = (history_next + 1) % HISTORY_SIZE;
    if (history_count < HISTORY_SIZE) history_count++;
}

/* Print history numbered 1..history_count, oldest -> newest */
void print_history(void) {
    if (history_count == 0) {
        printf("No history.\n");
        return;
    }
    int start = (history_next - history_count + HISTORY_SIZE) % HISTORY_SIZE;
    for (int i = 0; i < history_count; i++) {
        int idx = (start + i) % HISTORY_SIZE;
        printf("%d\t%s\n", i + 1, history_buf[idx]);
    }
}

/* Return strdup'd command for number n (1-based), or NULL if out of bounds */
char *get_history_cmd_by_number(int n) {
    if (n <= 0 || n > history_count) return NULL;
    int start = (history_next - history_count + HISTORY_SIZE) % HISTORY_SIZE;
    int idx = (start + (n - 1)) % HISTORY_SIZE;
    if (history_buf[idx] == NULL) return NULL;
    return strdup(history_buf[idx]);
}
