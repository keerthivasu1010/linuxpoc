#include <stdio.h>
#include <time.h>
#include "restart_policy.h"
#include "launcher.h"
#include "logger.h"

int restart_policy_evaluate(process_table_t *table)
{
    int triggered = 0;
    time_t now = time(NULL);

    for (process_t *proc = table->head; proc != NULL; proc = process_table_next(proc)) {

        if (proc->state != PROC_STATE_CRASHED) {
            continue; /* only crashed processes need a restart decision */
        }

        if (!proc->auto_restart) {
            /* operator explicitly disabled auto-restart for this service -
               leave it down, don't touch restart_count */
            continue;
        }

        int unlimited = (proc->max_restarts == -1);
        if (!unlimited && proc->restart_count >= proc->max_restarts) {
            proc->state = PROC_STATE_FAILED;
            logger_log(LOG_ERROR, "'%s' exceeded max_restarts (%d) - marking FAILED",
                       proc->name, proc->max_restarts);
            continue;
        }

        double elapsed = difftime(now, proc->last_exit_time);
        if (elapsed < proc->restart_backoff_sec) {
            /* backoff window still active - try again on a later tick,
               non-blocking, no sleep() here */
            continue;
        }

        proc->state = PROC_STATE_RESTARTING;
        proc->restart_count++;

        if (unlimited) {
            logger_log(LOG_WARN, "restarting '%s' (attempt %d/unlimited) after %.0fs backoff",
                       proc->name, proc->restart_count, elapsed);
        } else {
            logger_log(LOG_WARN, "restarting '%s' (attempt %d/%d) after %.0fs backoff",
                       proc->name, proc->restart_count, proc->max_restarts, elapsed);
        }

        if (launcher_start(proc) == 0) {
            triggered++;
        } else {
            /* fork() itself failed - treat as crashed again so we retry
               on the next tick rather than silently giving up */
            proc->state = PROC_STATE_CRASHED;
            logger_log(LOG_ERROR, "failed to relaunch '%s'", proc->name);
        }
    }

    return triggered;
}
