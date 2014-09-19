#include "conn.h"

void init_pool(int listenfd, pool *p) {
    int i;

    p->ndp = -1;
    p->nconn = 0;

    for (i = 0; i < FD_SETSIZE; i++) {
        p->clientfd[i] = -1;
    }

    FD_ZERO(&p->ready_set);
    FD_ZERO(&p->read_set);

    p->maxfd = listenfd;
    FD_SET(listenfd, &p->read_set);
}

void conn_handle(pool *p) {
    int i;

    for (i = 0; (i <= p->ndp) && (p->nconn > 0); i++) {
        int connfd, n;
        response_t resp;

        responseinit(&resp);

        connfd = p->clientfd[i];

        if ((connfd > 0) && (FD_ISSET(connfd, &p->ready_set))) {
            logging("--------------start handling connection from at socket %d------------\n", connfd);
            /*parse the request here*/
            p->nconn--;
            logging("Start Parsing Request\n");
            if (parseRequest(connfd, &resp) < 0) {
                logging("Parsing request error\n");
//                buildresp(connfd, &resp);
            }
            else{
                logging("Start building the response\n");
            }
            /*send the response here*/
            buildresp(connfd, &resp);

            if (resp.error == true) {
                /*close the connection*/
                logging("close connection %d\n", connfd);
                close(connfd);
                FD_CLR(connfd, &p->read_set);

                /*remove the fd from the pool*/
                p->clientfd[i] = -1;
                continue;
            }

            if (resp.path) {
                free(resp.path);
            }
            logging("-------------------connection handling finished----------------------\n");
        }
    }
}

void add_conn(int connfd, pool *p) {
    int i;
    p->nconn--;

    for (i = 0; i < FD_SETSIZE; i++) {
        if (p->clientfd[i] < 0) {
            p->clientfd[i] = connfd;

            FD_SET(connfd, &p->read_set);

            if (connfd > p->maxfd) {p->maxfd = connfd;}

            if (i > p->ndp) {p->ndp = i;}

            break;
        }

        if (i == FD_SETSIZE) {
            logging("add_conn error: Too many clients");
        }
    }
}
