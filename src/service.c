#include "service.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

const char *service_state_name(service_state_t state) {
    switch (state) {
        case SERVICE_STOPPED: return "STOPPED";
        case SERVICE_STARTING: return "STARTING";
        case SERVICE_RUNNING: return "RUNNING";
        case SERVICE_FAILED: return "FAILED";
        case SERVICE_STOPPING: return "STOPPING";
        default: return "UNKNOWN";
    }
}

int parse_service_line(const char *line, service_t *service) {
    char name[SERVICE_NAME_LEN];
    char exec_path[SERVICE_EXEC_LEN];
    int max_restarts, delay;

    if (!line || !service || line[0] == '#' || line[0] == '\n' || line[0] == '\0') return 0;
    if (sscanf(line, "%63s %255s %d %d", name, exec_path, &max_restarts, &delay) != 4) return -1;
    if (max_restarts < 0 || delay < 0) return -1;

    memset(service, 0, sizeof(*service));
    snprintf(service->name, sizeof(service->name), "%s", name);
    snprintf(service->executable, sizeof(service->executable), "%s", exec_path);
    service->pid = -1;
    service->state = SERVICE_STOPPED;
    service->max_restarts = max_restarts;
    service->restart_delay_sec = delay;
    return 1;
}
