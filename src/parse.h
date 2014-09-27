#ifndef PARSE_H
#define PARSE_H

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "log.h"
#include "io.h"

#define BUFSIZE 8193

typedef enum {
    OK, NOT_FOUND, BAD_REQUEST, LENGTH_REQUIRED, INTERNAL_SERVER_ERROR, NOT_IMPLEMENTED, SERVICE_UNAVAILABLE, HTTP_VERSION_NOT_SUPPORTED
} status_t;

typedef enum {
    CONNECTION_CLOSE, CONNECTION_ALIVE, TIME, SERVER, CONTENT_LEN, CONTENT_TYP, LAST_MDY
} field_t;

typedef enum {
    false,true
} bool;

typedef enum  {
    HTML, CSS, JPEG, PNG, GIF, OTHER,
}MIMEType;

typedef enum {
    GET, HEAD, POST, NOT_SUPPORT
} method_t;

typedef struct {
    bool error;
    method_t method;
    status_t status;
    off_t content_len;
    char *path;
    char uri[BUFSIZE];
    char header[BUFSIZE];
    char page[BUFSIZE];
    MIMEType filetype;
    time_t last_md;
} response_t;

typedef struct _conn_node{
    int connfd;
    bool isSSL;
    SSL* context;
    struct _conn_node* prev;
    struct _conn_node* next;
}conn_node;

int parseRequest(conn_node* node, response_t *resp);

void responseinit(response_t *resp);

#endif