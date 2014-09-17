#ifndef IO_H
#define IO_H

#include <unistd.h>
#include <string.h>
#include "log.h"

int initIO(char *lock_file, char *www_folder, char *cgi_path);

int readline(int fd, char *buf, int size);

char *getpath(char *file);

#endif