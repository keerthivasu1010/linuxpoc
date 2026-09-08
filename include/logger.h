#ifndef LOGGER_H
#define LOGGER_H

void logger_init(const char *path);
void logger_close(void);
void log_event(const char *level, const char *service, int pid, const char *fmt, ...);

#endif
