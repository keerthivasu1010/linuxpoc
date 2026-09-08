#ifndef SERVICE_H
#define SERVICE_H

#include <sys/types.h>

#define SERVICE_NAME_LEN 64
#define SERVICE_EXEC_LEN 256

typedef enum {
    SERVICE_STOPPED,
    SERVICE_STARTING,
    SERVICE_RUNNING,
    SERVICE_FAILED,
    SERVICE_STOPPING
} service_state_t;

typedef struct {
    char name[SERVICE_NAME_LEN];
    char executable[SERVICE_EXEC_LEN];
    pid_t pid;
    service_state_t state;
    int restart_count;
    int max_restarts;
    int restart_delay_sec;
} service_t;

const char *service_state_name(service_state_t state);
int parse_service_line(const char *line, service_t *service);

#endif
