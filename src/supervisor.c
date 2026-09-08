#define _POSIX_C_SOURCE 200809L
#include "supervisor.h"
#include "logger.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

void supervisor_init(supervisor_t *sup) {
    memset(sup, 0, sizeof(*sup));
    sup->running = 1;
    sup->shutdown_requested = 0;
    for (size_t i = 0; i < MAX_SERVICES; ++i) sup->services[i].pid = -1;
}

int supervisor_load_config(supervisor_t *sup, const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return -1;

    char line[512];
    int line_no = 0;
    while (fgets(line, sizeof(line), fp)) {
        ++line_no;
        service_t svc;
        int rc = parse_service_line(line, &svc);
        if (rc == 0) continue;
        if (rc < 0) {
            log_event("ERROR", NULL, 0, "Invalid configuration at line %d", line_no);
            fclose(fp);
            return -1;
        }
        if (sup->service_count >= MAX_SERVICES) {
            log_event("ERROR", NULL, 0, "Maximum service count (%d) exceeded", MAX_SERVICES);
            fclose(fp);
            return -1;
        }
        sup->services[sup->service_count++] = svc;
    }
    fclose(fp);
    return sup->service_count > 0 ? 0 : -1;
}

int supervisor_start_service(supervisor_t *sup, size_t index) {
    if (index >= sup->service_count) return -1;
    service_t *svc = &sup->services[index];

    struct stat st;
    if (stat(svc->executable, &st) != 0) {
        svc->state = SERVICE_FAILED;
        log_event("ERROR", svc->name, 0, "Executable not found: %s", svc->executable);
        return -1;
    }
    if (!S_ISREG(st.st_mode) || access(svc->executable, X_OK) != 0) {
        svc->state = SERVICE_FAILED;
        log_event("ERROR", svc->name, 0, "Executable is not runnable: %s", svc->executable);
        return -1;
    }

    svc->state = SERVICE_STARTING;
    pid_t pid = fork();
    if (pid < 0) {
        svc->state = SERVICE_FAILED;
        log_event("ERROR", svc->name, 0, "fork() failed: %s", strerror(errno));
        return -1;
    }

    if (pid == 0) {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sigemptyset(&sa.sa_mask);
        sa.sa_handler = SIG_DFL;
        sigaction(SIGTERM, &sa, NULL);
        sigaction(SIGINT, &sa, NULL);
        sigaction(SIGCHLD, &sa, NULL);
        execl(svc->executable, svc->executable, svc->name, (char *)NULL);
        dprintf(STDERR_FILENO, "exec failed for %s: %s\n", svc->name, strerror(errno));
        _exit(127);
    }

    svc->pid = pid;
    svc->state = SERVICE_RUNNING;
    log_event("INFO", svc->name, pid, "Service started");
    return 0;
}

static int restart_service(supervisor_t *sup, size_t index) {
    service_t *svc = &sup->services[index];
    if (svc->restart_count >= svc->max_restarts) {
        svc->state = SERVICE_FAILED;
        log_event("ERROR", svc->name, 0, "Restart limit reached (%d attempts)", svc->max_restarts);
        return -1;
    }

    ++svc->restart_count;
    log_event("WARN", svc->name, 0, "Restarting service, attempt=%d", svc->restart_count);
    if (svc->restart_delay_sec > 0) sleep((unsigned int)svc->restart_delay_sec);
    return supervisor_start_service(sup, index);
}

void supervisor_process_children(supervisor_t *sup) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        size_t index = MAX_SERVICES;
        for (size_t i = 0; i < sup->service_count; ++i) {
            if (sup->services[i].pid == pid) { index = i; break; }
        }
        if (index == MAX_SERVICES) {
            log_event("WARN", NULL, pid, "Unknown child reaped");
            continue;
        }

        service_t *svc = &sup->services[index];
        svc->pid = -1;

        if (WIFEXITED(status)) {
            int code = WEXITSTATUS(status);
            log_event(code == 0 ? "INFO" : "ERROR", svc->name, pid,
                      "Service exited, status=%d", code);
        } else if (WIFSIGNALED(status)) {
            log_event("ERROR", svc->name, pid,
                      "Service terminated by signal %d", WTERMSIG(status));
        }

        if (sup->running && !sup->shutdown_requested) {
            svc->state = SERVICE_FAILED;
            restart_service(sup, index);
        } else {
            svc->state = SERVICE_STOPPED;
        }
    }
}

void supervisor_print_status(const supervisor_t *sup) {
    printf("\n========== ADAS SERVICE STATUS ==========""\n");
    printf("%-20s %-8s %-12s %-10s\n", "SERVICE", "PID", "STATE", "RESTARTS");
    printf("------------------------------------------\n");
    for (size_t i = 0; i < sup->service_count; ++i) {
        const service_t *s = &sup->services[i];
        printf("%-20s %-8d %-12s %d/%d\n", s->name, (int)s->pid,
               service_state_name(s->state), s->restart_count, s->max_restarts);
    }
    printf("==========================================\n");
    fflush(stdout);
}

void supervisor_shutdown(supervisor_t *sup) {
    sup->running = 0;
    log_event("INFO", NULL, 0, "Supervisor shutdown requested");

    for (size_t i = 0; i < sup->service_count; ++i) {
        service_t *svc = &sup->services[i];
        if (svc->pid > 0) {
            svc->state = SERVICE_STOPPING;
            if (kill(svc->pid, SIGTERM) < 0 && errno != ESRCH)
                log_event("ERROR", svc->name, svc->pid, "kill(SIGTERM) failed: %s", strerror(errno));
        }
    }

    const int max_wait = 5;
    for (int t = 0; t < max_wait; ++t) {
        supervisor_process_children(sup);
        int alive = 0;
        for (size_t i = 0; i < sup->service_count; ++i) if (sup->services[i].pid > 0) alive = 1;
        if (!alive) break;
        sleep(1);
    }

    for (size_t i = 0; i < sup->service_count; ++i) {
        service_t *svc = &sup->services[i];
        if (svc->pid > 0) {
            log_event("WARN", svc->name, svc->pid, "Forcing shutdown with SIGKILL");
            kill(svc->pid, SIGKILL);
        }
    }

    while (waitpid(-1, NULL, 0) > 0) {}
    for (size_t i = 0; i < sup->service_count; ++i) {
        sup->services[i].pid = -1;
        sup->services[i].state = SERVICE_STOPPED;
    }
}
