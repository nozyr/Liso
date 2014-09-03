/******************************************************************************
* echo_server.c                                                               *
*                                                                             *
* Description: This file contains the C source code for an echo server.  The  *
*              server runs on a hard-coded port and simply write back anything*
*              sent to it by connected clients. The code is based on the      *
*              starter code provided and the I/O multiplex code on 15213      *
*                                                                             *
* Authors: Yurui Zhou <yuruiz@ece.cmu.edu                                     *
*          Wolf Richter <wolf@cs.cmu.edu>                                     *
*                                                                             *
*******************************************************************************/

#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

// #define ECHO_PORT 9999
#define BUFSIZE 100000

// int byte_cnt = 0;

typedef struct
{
    int maxfd;            // record the file descriptor with highest index
    fd_set read_set;      // the set prepared for select()
    fd_set ready_set;     // the set that actually set as select() parameter
    int nconn;            // the number of connection ready to read
    int ndp;              // the (mas index of descriptor stored in pool) - 1
    int clientfd[FD_SETSIZE];  // store the file descriptor
} pool;

int close_socket(int sock)
{
    if (close(sock))
    {
        fprintf(stderr, "Failed closing socket.\n");
        return 1;
    }

    return 0;
}

void init_pool(int listenfd, pool *p)
{
    p->ndp = -1;

    for (int i = 0; i < FD_SETSIZE; i++)
    { p->clientfd[i] = -1; }

    p->maxfd = listenfd;
    FD_ZERO(&p->read_set);
    FD_SET(listenfd, &p->read_set);
}

void add_conn(int connfd, pool *p)
{
    p->nconn--;

    for (int i = 0; i < FD_SETSIZE; i++)
    {
        if (p->clientfd[i] < 0)
        {
            p->clientfd[i] = connfd;

            FD_SET(connfd, &p->read_set);

            if (connfd > p->maxfd)
            { p->maxfd = connfd; }

            if (i > p->ndp)
            { p->ndp = i; }

            break;
        }

    if (i == FD_SETSIZE)
    { fprintf(stderr,"add_conn error: Too many clients"); }
    }
}

void echo(pool *p)
{
    int connfd, recv_byte_n;
    char buf[BUFSIZE];

    for (int i = 0; (i <= p->ndp) && (p->nconn > 0); i++)
    {
        connfd = p->clientfd[i];

        if ((connfd > 0) && (FD_ISSET(connfd, &p->ready_set)))
        {
            p->nconn--;

            while ((recv_byte_n = recv(connfd, buf, BUFSIZE, MSG_DONTWAIT)) > 0)
            {
                // printf("receive %d bytes on port %d\n", n, connfd);
                send(connfd, buf, recv_byte_n, 0);
            }

            if (recv_byte_n == 0)
            {
                close(connfd);
                FD_CLR(connfd, &p->read_set);
                p->clientfd[i] = -1;
            }

            // if (n < 0)
            // {
            //     printf("%s\n", strerror(errno));
            // }
        }
    }
}


int main(int argc, char *argv[])
{
    int listen_sock, client_sock, port;
    socklen_t conn_size;
    struct sockaddr_in addr, cli_addr;
    static pool conn_pool;

    port = atoi(argv[1]);

    fprintf(stdout, "----- Echo Server -----\n");

    if ((listen_sock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        fprintf(stderr, "Failed creating socket.\n");
        return EXIT_FAILURE;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_sock, (struct sockaddr *) &addr, sizeof(addr)))
    {
        close_socket(listen_sock);
        fprintf(stderr, "Failed binding socket.\n");
        return EXIT_FAILURE;
    }


    if (listen(listen_sock, 5))
    {
        close_socket(listen_sock);
        fprintf(stderr, "Error listening on socket.\n");
        return EXIT_FAILURE;
    }

    init_pool(listen_sock, &conn_pool);

    while (1)
    {
        conn_pool.ready_set = conn_pool.read_set;
        conn_pool.nconn = select(conn_pool.maxfd + 1, &conn_pool.ready_set, NULL, NULL, NULL);

        conn_size = sizeof(cli_addr);

        if (FD_ISSET(listen_sock, &conn_pool.ready_set))
        {
            client_sock = accept(listen_sock, (struct sockaddr *)&cli_addr, &conn_size);
            add_conn(client_sock, &conn_pool);
        }

        echo(&conn_pool);
    }

    close_socket(listen_sock);

    return EXIT_SUCCESS;
}
