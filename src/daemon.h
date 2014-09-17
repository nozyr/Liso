#ifndef DAEMON_H
#define DAEMON_H

#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <string.h>

void daemonize(char *lock_file);

#endif