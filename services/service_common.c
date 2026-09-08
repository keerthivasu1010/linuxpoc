#define _POSIX_C_SOURCE 200809L
#include "service_common.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t stop_requested = 0;
static void stop_handler(int sig) { (void)sig; stop_requested = 1; }

int run_adas_service(const char *service_name) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = stop_handler;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    int crash_after = 0;
    const char *env = getenv("ADAS_CRASH_AFTER");
    if (env) crash_after = atoi(env);

    printf("[%s] service started, PID=%d\n", service_name, (int)getpid());
    fflush(stdout);
    int seconds = 0;
    while (!stop_requested) {
        sleep(1);
        ++seconds;
        if (crash_after > 0 && seconds >= crash_after) {
            fprintf(stderr, "[%s] simulated failure\n", service_name);
            fflush(stderr);
            return 2;
        }
    }
    printf("[%s] graceful shutdown\n", service_name);
    fflush(stdout);
    return 0;
}
