/*
 * ADAS ECU Supervisor - main.c
 *
 * Mini "init"-style process supervisor. Launches services from a config
 * file, monitors them continuously, restarts crashed ones per policy,
 * prints a live status dashboard, and logs everything to file.
 *
 * Usage: ./adas_supervisor [config_path] [log_path]
 *   defaults: config/services.conf, logs/supervisor.log
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include "process_table.h"
#include "launcher.h"
#include "monitor.h"
#include "restart_policy.h"
#include "dashboard.h"
#include "logger.h"

#define DEFAULT_CONFIG_PATH   "config/services.conf"
#define DEFAULT_LOG_PATH      "logs/supervisor.log"
#define TICK_INTERVAL_SEC     1
#define DASHBOARD_INTERVAL_SEC 10   /* periodic auto-print cadence */
#define SHUTDOWN_GRACE_SEC    5     /* time given to children after SIGTERM before SIGKILL */

static volatile sig_atomic_t g_shutdown_requested = 0;

static void shutdown_handler(int signo)
{
    (void)signo;
    g_shutdown_requested = 1;
}

/* Creates a directory if it doesn't already exist. Tolerates EEXIST.
   (We got bitten by assuming logs/ existed during module testing -
   the supervisor itself must not make that assumption.) */
static int ensure_dir_exists(const char *path)
{
    if (mkdir(path, 0755) == 0) return 0;
    if (errno == EEXIST) return 0;
    return -1;
}

/* Sends a signal to every process currently marked RUNNING in the table. */
static void signal_all_running(process_table_t *table, int sig)
{
    for (process_t *p = table->head; p != NULL; p = process_table_next(p)) {
        if (p->state == PROC_STATE_RUNNING && p->pid > 0) {
            kill(p->pid, sig);
        }
    }
}

/* Returns 1 if at least one tracked process is still RUNNING. */
static int any_still_running(process_table_t *table)
{
    for (process_t *p = table->head; p != NULL; p = process_table_next(p)) {
        if (p->state == PROC_STATE_RUNNING) return 1;
    }
    return 0;
}

static void install_shutdown_handlers(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = shutdown_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

/*
 * graceful_shutdown:
 * SIGTERM to every running child, then poll (reaping as they exit)
 * for up to SHUTDOWN_GRACE_SEC seconds. Anything still alive after
 * that gets SIGKILL. This ensures the supervisor never hangs at
 * shutdown waiting on a misbehaving child.
 */
static void graceful_shutdown(process_table_t *table)
{
    logger_log(LOG_INFO, "shutdown requested - sending SIGTERM to all running services");
    signal_all_running(table, SIGTERM);

    for (int waited = 0; waited < SHUTDOWN_GRACE_SEC; waited++) {
        sleep(1);
        if (monitor_has_pending_exit()) {
            monitor_reap_exited(table);
        }
        if (!any_still_running(table)) {
            logger_log(LOG_INFO, "all services exited cleanly during shutdown");
            return;
        }
    }

    if (any_still_running(table)) {
        logger_log(LOG_WARN, "some services did not exit within %ds - sending SIGKILL",
                   SHUTDOWN_GRACE_SEC);
        signal_all_running(table, SIGKILL);
        sleep(1);
        if (monitor_has_pending_exit()) {
            monitor_reap_exited(table);
        }
    }
}

int main(int argc, char *argv[])
{
    const char *config_path = (argc > 1) ? argv[1] : DEFAULT_CONFIG_PATH;
    const char *log_path    = (argc > 2) ? argv[2] : DEFAULT_LOG_PATH;

    if (ensure_dir_exists("logs") != 0) {
        fprintf(stderr, "warning: could not create logs/ directory: %s\n", strerror(errno));
    }

    logger_init(log_path);
    logger_log(LOG_INFO, "==================================================");
    logger_log(LOG_INFO, "ADAS ECU Supervisor starting (config=%s)", config_path);

    process_table_t table;
    process_table_init(&table);

    monitor_init();
    dashboard_init();
    install_shutdown_handlers();

    int loaded = launcher_load_config(config_path, &table);
    if (loaded <= 0) {
        logger_log(LOG_ERROR, "no valid services loaded from '%s' - exiting", config_path);
        logger_close();
        return EXIT_FAILURE;
    }

    launcher_start_all(&table);
    dashboard_print(&table);

    int seconds_since_dashboard = 0;

    /* ---- main supervisor loop ---- */
    while (!g_shutdown_requested) {
        sleep(TICK_INTERVAL_SEC);
        seconds_since_dashboard += TICK_INTERVAL_SEC;

        if (monitor_has_pending_exit()) {
            monitor_reap_exited(&table);
        }

        restart_policy_evaluate(&table);

        if (dashboard_sigusr1_requested() || seconds_since_dashboard >= DASHBOARD_INTERVAL_SEC) {
            dashboard_print(&table);
            seconds_since_dashboard = 0;
        }
    }

    /* ---- shutdown path ---- */
    graceful_shutdown(&table);
    dashboard_print(&table);

    logger_log(LOG_INFO, "ADAS ECU Supervisor stopped");
    process_table_destroy(&table);
    logger_close();

    return EXIT_SUCCESS;
}
