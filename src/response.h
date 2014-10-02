#ifndef REPONSE_H
#define REPONSE_H

#include <time.h>
#include <stdio.h>
#include <sys/stat.h>
#include <openssl/ssl.h>
#include "io.h"
#include "string.h"
#include "log.h"
#include "parse.h"
#include "cgi.h"

int buildresp(conn_node* node, response_t *resp);
int writecontent(conn_node* node, char* buf, int length);
#endif