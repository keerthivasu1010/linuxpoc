#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>
#include "monitor.h"
#include "logger.h"

/*
 * volatile sig_atomic_t is the only type safe to touch inside a signal
 * handler without race conditions. The handler does NOTHING else -
 * no printf, no table manipulation - because those aren't async-signal-safe.
 * All real work happens in monitor_reap_exited(), called from the main loop.
 */
static volatile sig_atomic_t g_sigchld_flag = 0;

static void sigchld_handler(int signo)
{
    (void)signo;
    g_sigchld_flag = 1;
}

void monitor_init(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; /* restart interrupted syscalls like sleep()/read() */

    if (sigaction(SIGCHLD, &sa, NULL) < 0) {
        perror("[monitor] sigaction(SIGCHLD) failed");
    }
}

int monitor_has_pending_exit(void)
{
    if (g_sigchld_flag) {
        g_sigchld_flag = 0;
        return 1;
    }
    return 0;
}

int monitor_reap_exited(process_table_t *table)
{
    int reaped = 0;
    int status;
    pid_t pid;

    /* WNOHANG loop: drain every exited child in one pass so we never
       leave a zombie behind, even if several processes crashed at once */
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        process_t *proc = process_table_find_by_pid(table, pid);

        if (!proc) {
            /* Reaped a pid we're not tracking - shouldn't normally happen,
               but don't let it crash the supervisor */
            logger_log(LOG_WARN, "reaped untracked pid=%d", pid);
            continue;
        }

        proc->last_exit_time = time(NULL);

        int exit_code = 0;
        int was_clean = 0;

        if (WIFEXITED(status)) {
            exit_code = WEXITSTATUS(status);
            was_clean = (exit_code == 0);
            logger_log(was_clean ? LOG_INFO : LOG_WARN,
                       "'%s' (pid=%d) exited with code %d", proc->name, pid, exit_code);
        } else if (WIFSIGNALED(status)) {
            exit_code = -WTERMSIG(status); /* negative = "killed by signal N" */
            was_clean = 0;
            logger_log(LOG_WARN, "'%s' (pid=%d) killed by signal %d",
                       proc->name, pid, WTERMSIG(status));
        }

        proc->last_exit_code = exit_code;
        proc->pid = -1;

        /* A one-shot process (auto_restart=0) exiting cleanly is expected.
           Anything else - non-zero exit, signal kill, or a supposedly
           long-running service (auto_restart=1) exiting at all - counts
           as a crash needing the restart policy's attention. */
        if (was_clean && !proc->auto_restart) {
            proc->state = PROC_STATE_STOPPED;
        } else {
            proc->state = PROC_STATE_CRASHED;
        }

        reaped++;
    }

    return reaped;
}
