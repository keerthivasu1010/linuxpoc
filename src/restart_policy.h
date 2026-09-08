#ifndef RESTART_POLICY_H
#define RESTART_POLICY_H

#include "process_table.h"

/*
 * restart_policy_evaluate:
 * Scans the table for processes in PROC_STATE_CRASHED and decides
 * what to do with each one:
 *
 *   1. If auto_restart == 0            -> leave it CRASHED (no action;
 *                                          operator must handle manually)
 *   2. If restart_count >= max_restarts
 *      (and max_restarts != -1)        -> mark PROC_STATE_FAILED, stop
 *                                          trying permanently
 *   3. If backoff window hasn't elapsed
 *      yet (time(NULL) - last_exit_time
 *      < restart_backoff_sec)          -> leave it CRASHED, try again
 *                                          on a later call (non-blocking -
 *                                          we never sleep() in here)
 *   4. Otherwise                        -> increment restart_count,
 *                                          set state RESTARTING, call
 *                                          launcher_start()
 *
 * This is designed to be called once per supervisor main-loop tick,
 * alongside monitor_reap_exited(). It never blocks.
 *
 * Returns the number of restart attempts actually triggered in this call.
 */
int restart_policy_evaluate(process_table_t *table);

#endif /* RESTART_POLICY_H */
