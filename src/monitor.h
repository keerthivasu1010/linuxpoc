#ifndef MONITOR_H
#define MONITOR_H

#include "process_table.h"

/*
 * monitor_init:
 * Installs the SIGCHLD handler. Must be called once, after the
 * process table exists and before/after launching children (order
 * doesn't matter - the handler just sets a flag).
 */
void monitor_init(void);

/*
 * monitor_has_pending_exit:
 * Returns 1 if at least one child has exited since the last check
 * (i.e. SIGCHLD fired), 0 otherwise. Non-blocking, safe to poll in
 * a loop. Clears the internal flag as a side effect.
 */
int monitor_has_pending_exit(void);

/*
 * monitor_reap_exited:
 * Non-blocking reap of ALL children that have exited (uses
 * waitpid(-1, ..., WNOHANG) in a loop so it never misses a second
 * child exiting while we're processing the first).
 *
 * For each reaped pid found in the table:
 *   - records last_exit_time and last_exit_code
 *   - sets state to PROC_STATE_STOPPED if it was a clean, intentional
 *     exit (exit code 0 AND auto_restart == 0)
 *   - sets state to PROC_STATE_CRASHED otherwise (non-zero exit,
 *     killed by signal, or auto_restart is enabled so any exit is
 *     unexpected for a long-running service)
 *
 * Returns the number of children reaped in this call.
 */
int monitor_reap_exited(process_table_t *table);

#endif /* MONITOR_H */
