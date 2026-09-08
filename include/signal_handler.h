#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <signal.h>

extern volatile sig_atomic_t child_event;
extern volatile sig_atomic_t shutdown_requested;

int install_signal_handlers(void);

#endif
