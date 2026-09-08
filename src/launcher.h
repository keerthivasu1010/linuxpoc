#ifndef LAUNCHER_H
#define LAUNCHER_H

#include "process_table.h"

/*
 * launcher_load_config:
 * Reads a services.conf file and populates the process table with
 * one entry per valid line. Does NOT start any processes yet.
 *
 * Returns number of entries loaded, or -1 on fatal error (file not found).
 */
int launcher_load_config(const char *config_path, process_table_t *table);

/*
 * launcher_start:
 * fork() + execvp() a single process table entry.
 * On success, updates proc->pid, proc->state = PROC_STATE_RUNNING,
 * proc->last_start_time.
 * Returns 0 on success, -1 on failure (fork failed).
 */
int launcher_start(process_t *proc);

/*
 * launcher_start_all:
 * Calls launcher_start() for every entry currently in STOPPED,
 * CRASHED, or FAILED state. Used both at boot and for restarts.
 * Returns number of processes successfully started.
 */
int launcher_start_all(process_table_t *table);

#endif /* LAUNCHER_H */
