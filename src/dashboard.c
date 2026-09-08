#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include "dashboard.h"

static volatile sig_atomic_t g_sigusr1_flag = 0;

static void sigusr1_handler(int signo)
{
    (void)signo;
    g_sigusr1_flag = 1;
}

void dashboard_init(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigusr1_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGUSR1, &sa, NULL) < 0) {
        perror("[dashboard] sigaction(SIGUSR1) failed");
    }
}

int dashboard_sigusr1_requested(void)
{
    if (g_sigusr1_flag) {
        g_sigusr1_flag = 0;
        return 1;
    }
    return 0;
}

static const char *state_str(process_state_t s)
{
    switch (s) {
        case PROC_STATE_STOPPED:    return "STOPPED";
        case PROC_STATE_RUNNING:    return "RUNNING";
        case PROC_STATE_CRASHED:    return "CRASHED";
        case PROC_STATE_RESTARTING: return "RESTARTING";
        case PROC_STATE_FAILED:     return "FAILED";
    }
    return "UNKNOWN";
}

void dashboard_print(const process_table_t *table)
{
    time_t now = time(NULL);

    printf("\n================= ADAS SUPERVISOR STATUS =================\n");
    printf("%-14s %-8s %-11s %-10s %-8s %-6s\n",
           "NAME", "PID", "STATE", "UPTIME", "RESTARTS", "EXIT");
    printf("------------------------------------------------------------\n");

    for (process_t *p = table->head; p != NULL; p = process_table_next(p)) {
        char uptime_buf[16] = "-";
        char restarts_buf[16];
        char pid_buf[16];

        if (p->state == PROC_STATE_RUNNING && p->pid > 0) {
            double secs = difftime(now, p->last_start_time);
            snprintf(uptime_buf, sizeof(uptime_buf), "%.0fs", secs);
            snprintf(pid_buf, sizeof(pid_buf), "%d", p->pid);
        } else {
            snprintf(pid_buf, sizeof(pid_buf), "-");
        }

        if (p->max_restarts == -1) {
            snprintf(restarts_buf, sizeof(restarts_buf), "%d/inf", p->restart_count);
        } else {
            snprintf(restarts_buf, sizeof(restarts_buf), "%d/%d",
                      p->restart_count, p->max_restarts);
        }

        printf("%-14s %-8s %-11s %-10s %-8s %-6d\n",
               p->name,
               pid_buf,
               state_str(p->state),
               uptime_buf,
               restarts_buf,
               p->last_exit_code);
    }

    printf("============================================================\n");
    printf("Total services: %d\n\n", table->count);
}
