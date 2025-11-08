#include "shell.h"

/* Simple linked list of key=value pairs */
typedef struct var_s {
    char *name;
    char *value;
    struct var_s *next;
} var_t;

static var_t *vars_head = NULL;

void set_var(const char *name, const char *value) {
    if (!name) return;
    /* validate name: start with letter or underscore, then letters/digits/_ */
    if (!((isalpha((unsigned char)name[0]) || name[0] == '_'))) return;

    /* search existing */
    var_t *cur = vars_head;
    while (cur) {
        if (strcmp(cur->name, name) == 0) {
            free(cur->value);
            cur->value = strdup(value ? value : "");
            return;
        }
        cur = cur->next;
    }

    var_t *n = malloc(sizeof(var_t));
    n->name = strdup(name);
    n->value = strdup(value ? value : "");
    n->next = vars_head;
    vars_head = n;
}

char *get_var(const char *name) {
    if (!name) return NULL;
    var_t *cur = vars_head;
    while (cur) {
        if (strcmp(cur->name, name) == 0) {
            return strdup(cur->value); /* caller frees */
        }
        cur = cur->next;
    }
    return NULL;
}

void print_vars(void) {
    var_t *cur = vars_head;
    if (!cur) { printf("No variables defined.\n"); return; }
    while (cur) {
        printf("%s=%s\n", cur->name, cur->value);
        cur = cur->next;
    }
}

void free_vars(void) {
    var_t *cur = vars_head;
    while (cur) {
        var_t *next = cur->next;
        free(cur->name);
        free(cur->value);
        free(cur);
        cur = next;
    }
    vars_head = NULL;
}

/* Expand argv in-place: for each arg that starts with $, replace with variable value (or empty string if undefined).
   Support forms: $VARNAME and ${VARNAME} */
void expand_argv_inplace(char **argv) {
    if (!argv) return;
    for (int i = 0; argv[i] != NULL; ++i) {
        char *a = argv[i];
        if (a[0] != '$') continue;

        char *varname = NULL;
        if (a[1] == '{') {
            /* ${NAME} */
            char *end = strchr(a + 2, '}');
            if (!end) continue;
            int len = end - (a + 2);
            varname = malloc(len + 1);
            strncpy(varname, a + 2, len);
            varname[len] = '\0';
        } else {
            /* $NAME (upto end) */
            varname = strdup(a + 1);
        }

        char *val = get_var(varname); /* malloc'd or NULL */
        free(varname);
        free(argv[i]);
        if (val) {
            argv[i] = val; /* use value (caller will free later) */
        } else {
            argv[i] = strdup(""); /* undefined -> empty string */
        }
    }
}
