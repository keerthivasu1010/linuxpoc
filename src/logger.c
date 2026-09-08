#define _GNU_SOURCE
#include "logger.h"
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>

static int log_fd = -1;

void logger_init(const char *path) {
    log_fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
}

void logger_close(void) {
    if (log_fd >= 0) close(log_fd);
    log_fd = -1;
}

void log_event(const char *level, const char *service, int pid, const char *fmt, ...) {
    char message[1024];
    char line[1280];
    char timebuf[64];
    time_t now = time(NULL);
    struct tm tm_now;
    localtime_r(&now, &tm_now);
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &tm_now);

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(message, sizeof(message), fmt, ap);
    va_end(ap);

    int n;
    if (pid > 0) {
        n = snprintf(line, sizeof(line), "[%s] [%s] %s PID=%d %s\n",
                     timebuf, level, service ? service : "supervisor", pid, message);
    } else {
        n = snprintf(line, sizeof(line), "[%s] [%s] %s %s\n",
                     timebuf, level, service ? service : "supervisor", message);
    }

    if (log_fd >= 0) write(log_fd, line, (size_t)n);
    dprintf(STDOUT_FILENO, "%s", line);
}
