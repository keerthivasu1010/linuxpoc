#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "permissions.h"
#include "logger.h"

void permissions_apply_secure_umask(void)
{
    /* 0027 => new files: owner rw, group r, others nothing.
       Applies to the supervisor's own output (log file), not to the
       child processes it launches - execvp() replaces the child's
       image and permission checks on that binary are handled
       separately by permissions_check_executable(). */
    umask(0027);
}

int permissions_check_executable(const char *path)
{
    struct stat st;

    if (stat(path, &st) != 0) {
        logger_log(LOG_ERROR, "permission check failed for '%s': %s",
                   path, strerror(errno));
        return -1;
    }

    if (!S_ISREG(st.st_mode)) {
        logger_log(LOG_ERROR, "'%s' is not a regular file (mode=0%o) - refusing to launch",
                   path, st.st_mode & 07777);
        return -1;
    }

    if (!(st.st_mode & S_IXUSR)) {
        logger_log(LOG_ERROR, "'%s' is not executable by owner (mode=0%o) - refusing to launch",
                   path, st.st_mode & 07777);
        return -1;
    }

    if (st.st_mode & S_IWGRP) {
        logger_log(LOG_ERROR,
                   "'%s' is group-writable (mode=0%o) - refusing to launch "
                   "(a compromised group member could tamper with this ADAS binary)",
                   path, st.st_mode & 07777);
        return -1;
    }

    if (st.st_mode & S_IWOTH) {
        logger_log(LOG_ERROR,
                   "'%s' is world-writable (mode=0%o) - refusing to launch "
                   "(any local user could tamper with this ADAS binary)",
                   path, st.st_mode & 07777);
        return -1;
    }

    return 0;
}
