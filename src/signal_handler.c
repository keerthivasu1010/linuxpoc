#include "signal_handler.h"
#include <string.h>

volatile sig_atomic_t child_event = 0;
volatile sig_atomic_t shutdown_requested = 0;

static void handle_sigchld(int sig) {
    (void)sig;
    child_event = 1;
}

static void handle_shutdown(int sig) {
    (void)sig;
    shutdown_requested = 1;
}

int install_signal_handlers(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sa.sa_handler = handle_sigchld;
    if (sigaction(SIGCHLD, &sa, NULL) < 0) return -1;

    sa.sa_handler = handle_shutdown;
    if (sigaction(SIGINT, &sa, NULL) < 0) return -1;
    if (sigaction(SIGTERM, &sa, NULL) < 0) return -1;

    signal(SIGPIPE, SIG_IGN);
    return 0;
}
