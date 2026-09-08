#ifndef SUPERVISOR_H
#define SUPERVISOR_H

#include "service.h"
#include <stddef.h>
#include <signal.h>

#define MAX_SERVICES 16

typedef struct {
    service_t services[MAX_SERVICES];
    size_t service_count;
    volatile sig_atomic_t shutdown_requested;
    int running;
} supervisor_t;

void supervisor_init(supervisor_t *sup);
int supervisor_load_config(supervisor_t *sup, const char *path);
int supervisor_start_service(supervisor_t *sup, size_t index);
void supervisor_process_children(supervisor_t *sup);
void supervisor_print_status(const supervisor_t *sup);
void supervisor_shutdown(supervisor_t *sup);

#endif
