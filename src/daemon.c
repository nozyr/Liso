#include "daemon.h"


void daemonize(char *lock_file) {
    /*Create a child process from parent process*/
    int lockfd, log_start_len, pid = fork();
    char log_start[50] = {0};

    if (pid < 0) {
        printf("Fork Process Error");
        exit(EXIT_FAILURE);
    }

    /*Terminate the the parent process.*/
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    /*run the process in new session and have a new group*/
    if (setsid() < 0) {
        printf("set process seession error");
        exit(EXIT_FAILURE);
    }

    /*unmask the file mode*/
    umask(0);

    /*Create the lock file*/
    lockfd = open(lock_file, O_RDWR | O_CREAT | O_EXCL);

    if (lockfd < 0) {
        printf("Create Lock file error");
        exit(EXIT_FAILURE);
    }

    /*Lock the lock file*/
    if (lockf(lockfd, F_TLOCK, 0) < 0) {
        printf("lock file error");
        exit(EXIT_FAILURE);
    }

    /*Write to lockfile*/
    sprintf(log_start, "Liso start at pid %d\n", getpid());
    log_start_len = strlen(lockfd);
    if (log_start_len != write(lockfd, log_start, log_start_len)) {
        printf("Writing to log file error");
        exit(EXIT_FAILURE);
    }

    /*close stdin, stdout and stderr*/
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}