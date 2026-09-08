#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "process_table.h"

void process_table_init(process_table_t *table)
{
    table->head = NULL;
    table->count = 0;
}

process_t *process_table_add(process_table_t *table,
                              const char *name,
                              const char *path,
                              char *const argv[],
                              int argc,
                              int auto_restart,
                              int max_restarts,
                              int restart_backoff_sec)
{
    process_t *node = calloc(1, sizeof(process_t));
    if (!node) {
        perror("calloc failed for process_t");
        return NULL;
    }

    strncpy(node->name, name, MAX_NAME_LEN - 1);
    strncpy(node->path, path, MAX_PATH_LEN - 1);

    node->argc = (argc > MAX_ARGS) ? MAX_ARGS : argc;
    for (int i = 0; i < node->argc; i++) {
        node->argv[i] = strdup(argv[i]);
    }
    node->argv[node->argc] = NULL; /* execvp requires NULL-terminated argv */

    node->pid = -1;
    node->state = PROC_STATE_STOPPED;
    node->auto_restart = auto_restart;
    node->max_restarts = max_restarts;
    node->restart_count = 0;
    node->restart_backoff_sec = restart_backoff_sec;
    node->last_start_time = 0;
    node->last_exit_time = 0;
    node->last_exit_code = 0;

    /* insert at head - order doesn't matter for a supervisor table */
    node->next = table->head;
    table->head = node;
    table->count++;

    return node;
}

process_t *process_table_find_by_name(process_table_t *table, const char *name)
{
    for (process_t *cur = table->head; cur != NULL; cur = cur->next) {
        if (strcmp(cur->name, name) == 0) {
            return cur;
        }
    }
    return NULL;
}

process_t *process_table_find_by_pid(process_table_t *table, pid_t pid)
{
    if (pid <= 0) return NULL;
    for (process_t *cur = table->head; cur != NULL; cur = cur->next) {
        if (cur->pid == pid) {
            return cur;
        }
    }
    return NULL;
}

process_t *process_table_next(process_t *current)
{
    return current ? current->next : NULL;
}

void process_table_destroy(process_table_t *table)
{
    process_t *cur = table->head;
    while (cur != NULL) {
        process_t *tmp = cur;
        cur = cur->next;
        for (int i = 0; i < tmp->argc; i++) {
            free(tmp->argv[i]);
        }
        free(tmp);
    }
    table->head = NULL;
    table->count = 0;
}
