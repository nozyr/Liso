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

// #define ECHO_PORT 9999
#define BUFSIZE 8192

int byte_cnt = 0;

typedef struct
{
    int maxfd;
    fd_set read_set;
    fd_set ready_set;
    int nready;
    int maxi;
    int clientfd[FD_SETSIZE];
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
    int i;
    p->maxi = -1;

    for (i = 0; i < FD_SETSIZE; i++)
    { p->clientfd[i] = -1; }

    p->maxfd = listenfd;
    FD_ZERO(&p->read_set);
    FD_SET(listenfd, &p->read_set);
}

void add_client(int connfd, pool *p)
{
    int i;
    p->nready--;

    for (i = 0; i < FD_SETSIZE; i++)  /* Find an available slot */
        if (p->clientfd[i] < 0)
        {
            /* Add connected descriptor to the pool */
            p->clientfd[i] = connfd;

            /* Add the descriptor to descriptor set */
            FD_SET(connfd, &p->read_set);

            /* Update max descriptor and pool highwater mark */
            if (connfd > p->maxfd)
            { p->maxfd = connfd; }

            if (i > p->maxi)
            { p->maxi = i; }

            break;
        }

    if (i == FD_SETSIZE) /* Couldn't find an empty slot */
    { fprintf(stderr,"add_client error: Too many clients"); }
}

void check_clients(pool *p)
{
    int i, connfd, n;
    char buf[BUFSIZE];

    for (i = 0; (i <= p->maxi) && (p->nready > 0); i++)
    {
        connfd = p->clientfd[i];

        /* If the descriptor is ready, echo a text line from it */
        if ((connfd > 0) && (FD_ISSET(connfd, &p->ready_set)))
        {
            p->nready--;

            if ((n = read(connfd, buf, BUFSIZE)) != 0)
            {
                byte_cnt += n;
                printf("Server received %d (%d total) bytes on fd %d\n",
                       n, byte_cnt, connfd);
                write(connfd, buf, n);
            }

            /* EOF detected, remove descriptor from pool */
            else
            {
                close(connfd);
                FD_CLR(connfd, &p->read_set);
                p->clientfd[i] = -1;
            }
        }
    }
}


int main(int argc, char *argv[])
{
    int listen_sock, client_sock, port;
    socklen_t cli_size;
    struct sockaddr_in addr, cli_addr;
    static pool conn_pool;

    port = atoi(argv[1]);

    fprintf(stdout, "----- Echo Server -----\n");

    /* all networked programs must create a socket */
    if ((listen_sock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        fprintf(stderr, "Failed creating socket.\n");
        return EXIT_FAILURE;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    /* servers bind sockets to ports---notify the OS they accept connections */
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

    /* finally, loop waiting for input and then write it back */
    while (1)
    {
        /* Wait for listening/connected descriptor(s) to become ready */
        conn_pool.ready_set = conn_pool.read_set;
        conn_pool.nready = select(conn_pool.maxfd + 1, &conn_pool.ready_set, NULL, NULL, NULL);

        cli_size = sizeof(cli_addr);

        /* If listening descriptor ready, add new client to conn_pool */
        if (FD_ISSET(listen_sock, &conn_pool.ready_set))
        {
            client_sock = accept(listen_sock, (struct sockaddr *)&cli_addr, &cli_size);
            add_client(client_sock, &conn_pool);
        }

        /* Echo a text line from each ready connected descriptor */
        check_clients(&conn_pool);
    }

    close_socket(listen_sock);

    return EXIT_SUCCESS;
}
