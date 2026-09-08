#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include "logger.h"

static FILE *g_log_fp = NULL;

int logger_init(const char *log_path)
{
    g_log_fp = fopen(log_path, "a");
    if (!g_log_fp) {
        perror("[logger] failed to open log file - continuing with stdout/stderr only");
        return -1;
    }
    /* line-buffered so log lines hit disk promptly, important for a
       supervisor that might crash or be killed - we want the trail
       up to that point, not lost in a stdio buffer */
    setvbuf(g_log_fp, NULL, _IOLBF, 0);
    return 0;
}

static const char *level_str(log_level_t level)
{
    switch (level) {
        case LOG_INFO:  return "INFO";
        case LOG_WARN:  return "WARN";
        case LOG_ERROR: return "ERROR";
    }
    return "?";
}

void logger_log(log_level_t level, const char *fmt, ...)
{
    time_t now = time(NULL);
    char timebuf[32];
    struct tm tm_now;
    localtime_r(&now, &tm_now);
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &tm_now);

    /* Build the message once, then write it to both destinations so
       file and console never disagree due to a formatting bug in one
       path but not the other. */
    char msgbuf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msgbuf, sizeof(msgbuf), fmt, args);
    va_end(args);

    FILE *console = (level == LOG_ERROR) ? stderr : stdout;
    fprintf(console, "[%s] [%s] %s\n", timebuf, level_str(level), msgbuf);

    if (g_log_fp) {
        fprintf(g_log_fp, "[%s] [%s] %s\n", timebuf, level_str(level), msgbuf);
    }
}

void logger_close(void)
{
    if (g_log_fp) {
        fflush(g_log_fp);
        fclose(g_log_fp);
        g_log_fp = NULL;
    }
}
