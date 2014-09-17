#ifndef CONN_H
#define CONN_H

#include <vproc.h>
#include <stdio.h>
#include "parse.h"
#include "response.h"

typedef struct {
    int maxfd;            // record the file descriptor with highest index
    fd_set read_set;      // the set prepared for select()
    fd_set ready_set;     // the set that actually set as select() parameter
    int nconn;            // the number of connection ready to read
    int ndp;              // the (mas index of descriptor stored in pool) - 1
    int clientfd[FD_SETSIZE];  // store the file descriptor
} pool;

void init_pool(int listenfd, pool *p);

void add_conn(int connfd, pool *p);

void conn_handle(pool *p);

#endif