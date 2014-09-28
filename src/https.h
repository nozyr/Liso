#ifndef HTTPS_H
#define HTTPS_H
#include <stdlib.h>
#include <openssl/ssl.h>
#include <stdio.h>
#include <unistd.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include "log.h"

SSL_CTX * sslinit(char *key, char *crt);

#endif