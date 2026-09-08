#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include "launcher.h"
#include "logger.h"
#include "permissions.h"

#define MAX_LINE_LEN 512

/* strip leading/trailing whitespace in-place, return pointer to trimmed start */
static char *trim(char *s)
{
    while (isspace((unsigned char)*s)) s++;
    if (*s == '\0') return s;

    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return s;
}

/*
 * Parses one config line of the form:
 *   name : path : arg1,arg2 : auto_restart : max_restarts : backoff_sec
 * and adds it to the table. Returns 1 on success, 0 if the line
 * was blank/comment, -1 on malformed line.
 */
static int parse_and_add_line(char *line, process_table_t *table)
{
    char *trimmed = trim(line);
    if (trimmed[0] == '\0' || trimmed[0] == '#') {
        return 0; /* blank or comment */
    }

    char *fields[6] = {0};
    int nfields = 0;
    char *saveptr = NULL;
    char *tok = strtok_r(trimmed, ":", &saveptr);
    while (tok != NULL && nfields < 6) {
        fields[nfields++] = trim(tok);
        tok = strtok_r(NULL, ":", &saveptr);
    }

    if (nfields != 6) {
        logger_log(LOG_WARN, "malformed config line (expected 6 fields, got %d): %s",
                   nfields, trimmed);
        return -1;
    }

    const char *name       = fields[0];
    const char *path       = fields[1];
    char *args_str          = fields[2];
    int auto_restart       = atoi(fields[3]);
    int max_restarts       = atoi(fields[4]);
    int backoff_sec        = atoi(fields[5]);

    /* build argv: argv[0] = path, then comma-split args */
    char *argv[MAX_ARGS];
    int argc = 0;
    argv[argc++] = (char *)path;

    if (args_str[0] != '\0') {
        char *asave = NULL;
        char *atok = strtok_r(args_str, ",", &asave);
        while (atok != NULL && argc < MAX_ARGS - 1) {
            argv[argc++] = trim(atok);
            atok = strtok_r(NULL, ",", &asave);
        }
    }

    process_t *proc = process_table_add(table, name, path, argv, argc,
                                         auto_restart, max_restarts, backoff_sec);
    return proc ? 1 : -1;
}

int launcher_load_config(const char *config_path, process_table_t *table)
{
    FILE *fp = fopen(config_path, "r");
    if (!fp) {
        perror("[launcher] failed to open config file");
        return -1;
    }

    char line[MAX_LINE_LEN];
    int loaded = 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
        int rc = parse_and_add_line(line, table);
        if (rc == 1) loaded++;
        /* rc == 0 (blank/comment) and rc == -1 (malformed) are both non-fatal;
           malformed lines are logged and skipped so one bad line doesn't
           take down the whole config load */
    }

    fclose(fp);
    logger_log(LOG_INFO, "loaded %d service(s) from %s", loaded, config_path);
    return loaded;
}

int launcher_start(process_t *proc)
{
    /* Level 5 - Preserve Permissions: refuse to launch anything that
       fails the safety check, whether this is the first launch or a
       restart after a crash. A tampered binary getting re-exec'd
       automatically by the restart policy would be worse than one
       that never launched at all. */
    if (permissions_check_executable(proc->path) != 0) {
        proc->state = PROC_STATE_FAILED;
        return -1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        logger_log(LOG_ERROR, "fork() failed for '%s': %s", proc->name, strerror(errno));
        proc->state = PROC_STATE_FAILED;
        return -1;
    }

    if (pid == 0) {
        /* child process: replace image with the target executable.
           NOTE: logger_log() is stdio-based and NOT async-signal-safe,
           but that's fine here - after fork() but before execvp() we
           are still a normal (if duplicated) process, not inside a
           signal handler, so stdio is safe to use right up until the
           execvp() call replaces this image. */
        execvp(proc->path, proc->argv);

        /* execvp only returns on failure */
        logger_log(LOG_ERROR, "execvp failed for '%s': %s", proc->name, strerror(errno));
        _exit(127); /* standard "command not found/exec failed" code */
    }

    /* parent: record the child's pid and mark it running */
    proc->pid = pid;
    proc->state = PROC_STATE_RUNNING;
    proc->last_start_time = time(NULL);
    logger_log(LOG_INFO, "started '%s' (pid=%d)", proc->name, pid);
    return 0;
}

int launcher_start_all(process_table_t *table)
{
    int started = 0;
    for (process_t *cur = table->head; cur != NULL; cur = process_table_next(cur)) {
        if (cur->state == PROC_STATE_STOPPED ||
            cur->state == PROC_STATE_CRASHED ||
            cur->state == PROC_STATE_FAILED) {
            if (launcher_start(cur) == 0) {
                started++;
            }
        }
    }
    return started;
}
