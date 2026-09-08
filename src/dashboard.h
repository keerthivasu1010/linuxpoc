#ifndef DASHBOARD_H
#define DASHBOARD_H

#include "process_table.h"

/*
 * dashboard_print:
 * Prints a formatted snapshot of the entire process table to stdout -
 * name, pid, state, uptime (if running), restart count, last exit code.
 *
 * Safe to call from the main loop on a timer (e.g. every N seconds)
 * or on-demand (e.g. triggered by SIGUSR1 for an operator to request
 * a status check without stopping the supervisor).
 */
void dashboard_print(const process_table_t *table);

/*
 * dashboard_sigusr1_requested:
 * Returns 1 if SIGUSR1 was received since the last check (operator
 * asking for an on-demand status dump), 0 otherwise. Clears the flag.
 * Call dashboard_init() once before using this.
 */
int dashboard_sigusr1_requested(void);

/* Installs the SIGUSR1 handler for on-demand dashboard requests. */
void dashboard_init(void);

#endif /* DASHBOARD_H */
