#include "conn.h"

void init_pool(int http_fd, int https_fd, pool *p) {
//    int i;

//    p->ndp = -1;
//    p->nconn = 0;
    p->list_head = NULL;
    p->list_tail = NULL;

//    for (i = 0; i < FD_SETSIZE; i++) {
//        p->clientfd[i] = -1;
//    }

    FD_ZERO(&p->ready_set);
    FD_ZERO(&p->read_set);

    p->maxfd = http_fd > https_fd ? http_fd : https_fd;
    FD_SET(http_fd, &p->read_set);
    FD_SET(https_fd, &p->read_set);
}

void remove_node(conn_node *node) {
    if (node->prev != NULL) {
        node->prev->next = node->next;
    }

    if (node->next != NULL) {
        node->next->prev = node->prev;
    }
}

void conn_handle(pool *p) {
    conn_node *cur_node;
    cur_node = p->list_head;

    logging("Start Connection Handling procedure\n");

    /*Handle the http connections*/
    while (cur_node != NULL && p->nconn > 0) {

        logging("Now in the handling loop\n");
        if ((cur_node->connfd > 0) && (FD_ISSET(cur_node->connfd, &p->ready_set))) {
            response_t resp;
            responseinit(&resp);
            p->nconn--;
            logging("--------------start handling http connection from at socket %d------------\n", cur_node->connfd);
            /*parse the request here*/
            logging("Start Parsing Request\n");
            if (parseRequest(cur_node, &resp) < 0) {
                logging("Parsing request error\n");
            }
            else {
                logging("Start building the response\n");
            }
            /*send the response here*/
            buildresp(cur_node, &resp);

            if (resp.error == true) {
                /*close the connection*/
                logging("close connection %d\n", cur_node->connfd);
                close(cur_node->connfd);
                FD_CLR(cur_node->connfd, &p->read_set);

                /*remove node from the connection list*/
                remove_node(cur_node);
                free(cur_node);
                p->ndp--;
                continue;
            }

            if (resp.path) {
                free(resp.path);
            }
            logging("-------------------connection handling finished----------------------\n");
        }

        cur_node = cur_node->next;
    }
}

int add_conn(int connfd, pool *p) {
    p->nconn--;

    if (p->ndp == FD_SETSIZE) {
        logging("add_conn error: Too many clients");
        return -1;
    }

    p->ndp++;

    if (p->maxfd < connfd) {
        p->maxfd = connfd;
    }

    FD_SET(connfd, &p->read_set);

    conn_node *new_node = malloc(sizeof(conn_node));

    new_node->connfd = connfd;
    new_node->isSSL = false;
    new_node->context = NULL;
    new_node->prev = NULL;
    new_node->next = NULL;

    if (p->list_head == NULL) {
        p->list_head = new_node;
        p->list_tail = new_node;
    }
    else {
        p->list_tail->next = new_node;
        new_node->prev = p->list_tail;
        p->list_tail = new_node;
    }

    return 0;
}

int add_ssl(int connfd, pool *p, SSL_CTX *ssl_context) {
    SSL *client_context;
    p->nconn--;

    if (p->ndp == FD_SETSIZE) {
        logging("add_conn error: Too many clients");
        return -1;
    }

    p->ndp++;

    if (p->maxfd < connfd) {
        p->maxfd = connfd;
    }

    if ((client_context = SSL_new(ssl_context)) == NULL) {
        logging("Error creating client SSL context.\n");
        return -1;
    }

    if (SSL_set_fd(client_context, connfd) == 0) {
        logging("Error creating client SSL context.\n");
        return -1;
    }

    if (SSL_accept(client_context) <= 0) {
        logging("Error accepting (handshake) client SSL context.\n");
        return -1;
    }

    FD_SET(connfd, &p->read_set);
    conn_node *new_node = malloc(sizeof(conn_node));
    new_node->connfd = connfd;
    new_node->isSSL = true;
    new_node->context = client_context;
    new_node->next = NULL;
    new_node->prev = NULL;

    if (p->list_head == NULL) {
        p->list_head = new_node;
        p->list_tail = new_node;
    }
    else {
        p->list_tail->next = new_node;
        new_node->prev = p->list_tail;
        p->list_tail = new_node;
    }

    return 0;
}