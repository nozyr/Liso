#ifndef PARSE_H
#define PARSE_H

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "log.h"
#include "io.h"

#define BUFSIZE 8193

typedef enum {
    OK, NOT_FOUND, LENGTH_REQUIRED, INTERNAL_SERVER_ERROR, NOT_IMPLEMENTED, SERVICE_UNAVAILABLE, HTTP_VERSION_NOT_SUPPORTED
} status_t;

typedef enum {
    CONNECTION_CLOSE, CONNECTION_ALIVE, TIME, SERVER, CONTENT_LEN, CONTENT_TYP, LAST_MDY
} field_t;

typedef enum{
    true, false
}bool;

typedef enum{
    GET, HEAD, POST, NOT_SUPPORT
}method_t;

typedef struct{
    bool version;
    bool error;
    method_t method;
    status_t status;
    char uri[BUFSIZE];
    char header[BUFSIZE];
}response_t;

int parseRequest(int connfd, response_t response);

void responseinit(response_t response);

#endif