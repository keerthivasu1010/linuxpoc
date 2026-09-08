#ifndef LOGGER_H
#define LOGGER_H

typedef enum {
    LOG_INFO = 0,
    LOG_WARN,
    LOG_ERROR
} log_level_t;

/*
 * logger_init:
 * Opens (or creates) the log file at log_path in append mode.
 * Must be called once at supervisor startup, before any logger_log() calls.
 * Returns 0 on success, -1 on failure (falls back to stderr-only logging).
 */
int logger_init(const char *log_path);

/*
 * logger_log:
 * Writes a single timestamped log line in the form:
 *   [YYYY-MM-DD HH:MM:SS] [LEVEL] message
 * to both the log file (if open) and stdout/stderr, using printf-style
 * formatting. Async-signal-safety is NOT required here because this is
 * only ever called from the main loop, never from inside a signal handler.
 */
void logger_log(log_level_t level, const char *fmt, ...);

/* Flushes and closes the log file. Call once at supervisor shutdown. */
void logger_close(void);

#endif /* LOGGER_H */
