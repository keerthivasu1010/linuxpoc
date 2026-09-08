#ifndef PERMISSIONS_H
#define PERMISSIONS_H

/*
 * permissions_apply_secure_umask:
 * Sets a restrictive umask (0027) for the supervisor process itself, so
 * any file it creates (logs, etc.) is not group-writable or world-
 * accessible. Should be called once, early in main(), before logger_init()
 * so the log file itself inherits the safer permissions.
 */
void permissions_apply_secure_umask(void);

/*
 * permissions_check_executable:
 * Verifies a service binary is safe to launch before we fork()/execvp()
 * it. On an ECU, silently running a tampered or misconfigured binary is
 * a safety issue, not just a bug - so this check runs before every
 * launch, not just at boot.
 *
 * Checks performed:
 *   1. Path exists and is a regular file (not a symlink to something
 *      unexpected, not a directory/device).
 *   2. Owner has execute permission.
 *   3. NOT group-writable and NOT world-writable - if any other user/
 *      group could modify this binary, we refuse to run it, since that
 *      would let a compromised low-privilege account replace a critical
 *      ADAS service undetected.
 *
 * Returns 0 if safe to launch, -1 otherwise (with details logged).
 */
int permissions_check_executable(const char *path);

#endif /* PERMISSIONS_H */
