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
        int connfd;
        response_t resp;

        responseinit(resp);

        connfd = p->clientfd[i];

        if ((connfd > 0) && (FD_ISSET(connfd, &p->ready_set))) {
            /*parse the request here*/
            if (parseRequest(connfd, resp) == -1) {
                response(connfd, resp);
                continue;
            }

            if (!resp.version) {
                response(connfd, resp);
                continue;
            }

            switch (resp.method) {
                case GET:
                    break;
                case HEAD:
                    break;
                case POST:
                    break;
                default:
                    break;
            }
            /*send the response here*/
            response(connfd, resp);

            if (resp.error) {
                /*close the connection*/
                close(connfd);

                /*remove the fd from the pool*/
                p->clientfd[i] = -1;
            }
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

        if (i == FD_SETSIZE) {fprintf(stderr, "add_conn error: Too many clients");}
    }
}
