#ifndef REPONSE_H
#define REPONSE_H

#include "io.h"
#include "string.h"
#include "log.h"
#include "parse.h"
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <openssl/ssl.h>

int buildresp(conn_node* node, response_t *resp);

#endif