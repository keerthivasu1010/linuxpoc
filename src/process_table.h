#ifndef PROCESS_TABLE_H
#define PROCESS_TABLE_H

#include <sys/types.h>
#include <time.h>

#define MAX_NAME_LEN     64
#define MAX_PATH_LEN     256
#define MAX_ARGS         16
#define MAX_ARG_LEN      128

/* Lifecycle status of a managed process */
typedef enum {
    PROC_STATE_STOPPED = 0,   /* not yet launched / intentionally stopped */
    PROC_STATE_RUNNING,       /* currently alive */
    PROC_STATE_CRASHED,       /* exited abnormally, awaiting restart decision */
    PROC_STATE_RESTARTING,    /* restart in progress */
    PROC_STATE_FAILED         /* exceeded max restart attempts, permanently down */
} process_state_t;

/*
 * process_t: one entry in the supervisor's process table.
 * This struct is the single source of truth for every module
 * (launcher, monitor, restart policy, dashboard, logger).
 */
typedef struct process {
    char name[MAX_NAME_LEN];          /* logical service name, e.g. "lane_detect" */
    char path[MAX_PATH_LEN];          /* absolute/relative path to executable */
    char *argv[MAX_ARGS];             /* argv array for execvp() (argv[0] = path) */
    int  argc;

    pid_t pid;                        /* current OS pid, -1 if not running */
    process_state_t state;

    int  auto_restart;                /* 1 = restart on crash, 0 = leave dead */
    int  max_restarts;                /* cap on restart attempts, -1 = unlimited */
    int  restart_count;               /* attempts made so far */
    int  restart_backoff_sec;         /* base delay before next restart */

    time_t last_start_time;
    time_t last_exit_time;
    int  last_exit_code;

    struct process *next;             /* singly linked list */
} process_t;

/* The whole table is just a head pointer + count, kept together */
typedef struct {
    process_t *head;
    int count;
} process_table_t;

/* Lifecycle of the table itself */
void process_table_init(process_table_t *table);
void process_table_destroy(process_table_t *table);

/* Add a new process entry (state defaults to STOPPED, pid = -1) */
process_t *process_table_add(process_table_t *table,
                              const char *name,
                              const char *path,
                              char *const argv[],
                              int argc,
                              int auto_restart,
                              int max_restarts,
                              int restart_backoff_sec);

/* Lookups */
process_t *process_table_find_by_name(process_table_t *table, const char *name);
process_t *process_table_find_by_pid(process_table_t *table, pid_t pid);

/* Iteration helper (returns next node, NULL when done) */
process_t *process_table_next(process_t *current);

#endif /* PROCESS_TABLE_H */
