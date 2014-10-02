#ifndef DAEMON_H
#define DAEMON_H

#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "log.h"

int daemonize(char *lock_file);

#endif