#define _POSIX_C_SOURCE 200809L
#include "supervisor.h"
#include "signal_handler.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

int main(int argc, char *argv[]) {
    const char *config = argc > 1 ? argv[1] : "config/services.conf";
    const char *log_path = argc > 2 ? argv[2] : "logs/supervisor.log";

    logger_init(log_path);
    supervisor_t sup;
    supervisor_init(&sup);

    if (install_signal_handlers() < 0) {
        log_event("ERROR", NULL, 0, "Failed to install signal handlers");
        logger_close();
        return EXIT_FAILURE;
    }
    if (supervisor_load_config(&sup, config) < 0) {
        log_event("ERROR", NULL, 0, "Failed to load configuration: %s", config);
        logger_close();
        return EXIT_FAILURE;
    }

    log_event("INFO", NULL, 0, "Starting ADAS ECU Process Supervisor with %zu services", sup.service_count);
    for (size_t i = 0; i < sup.service_count; ++i) supervisor_start_service(&sup, i);
    supervisor_print_status(&sup);

    int tick = 0;
    while (!shutdown_requested) {
        if (child_event) {
            child_event = 0;
            supervisor_process_children(&sup);
            supervisor_print_status(&sup);
        }
        if (++tick >= 5) {
            tick = 0;
            supervisor_print_status(&sup);
        }
        sleep(1);
    }

    sup.shutdown_requested = 1;
    supervisor_shutdown(&sup);
    supervisor_print_status(&sup);
    log_event("INFO", NULL, 0, "Supervisor exited cleanly");
    logger_close();
    return EXIT_SUCCESS;
}
