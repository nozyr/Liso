#include "conn.h"
#include "parse.h"

static void addCGI(cgi_node *node, pool* p);
static void removeCGI(cgi_node *node, pool* p);

void init_pool(int http_fd, int https_fd, pool *p) {

    p->list_head = NULL;
    p->list_tail = NULL;
    p->cgi_head = NULL;
    p->cgi_tail = NULL;

    FD_ZERO(&p->ready_set);
    FD_ZERO(&p->read_set);

    p->maxfd = http_fd > https_fd ? http_fd : https_fd;
    FD_SET(http_fd, &p->read_set);
    FD_SET(https_fd, &p->read_set);
}

void remove_node(conn_node *node, pool* p) {
    if (node->prev != NULL) {
        node->prev->next = node->next;
    }
    else{
        p->list_head = node->next;
        if (node->next != NULL) {
            node->next->prev = NULL;
        }
    }

    if (node->next != NULL) {
        node->next->prev = node->prev;
    }
    else{
        p->list_tail = node->prev;
        if (node->prev != NULL) {
            node->prev->next = NULL;
        }
    }

    free(node);
}

void freelist(hdNode *head){
    hdNode* prev = head;
    hdNode* cur = head;

    while (cur != NULL) {
        cur = cur->next;
        free(prev);
        prev = cur;
    }
}

void conn_handle(pool *p) {
    conn_node *cur_node;
    cgi_node* cur_cgi;
    cur_node = p->list_head;
    cur_cgi = p->cgi_head;

    logging("Start Connection Handling procedure\n");

    /*Handle the http connections*/
    while (cur_node != NULL && p->nconn > 0) {

//        logging("Now in the http handling loop\n");
        if (FD_ISSET(cur_node->connfd, &p->ready_set)) {
            conn_node* tormNode = NULL;
            logging("--------------start handling http connection from at socket %d------------\n", cur_node->connfd);

            response_t resp;
            responseinit(&resp);
            p->nconn--;

            resp.ishttps = cur_node->isSSL;
            resp.addr = cur_node->addr;

            /*parse the request here*/
            logging("Start Parsing Request\n");
            if (parseRequest(cur_node, &resp) < 0) {
                logging("Parsing request error\n");
            }
            else {
                logging("Start building the response\n");
            }
            /*send the response here*/
            if(resp.conn_close == false){
                if (buildresp(cur_node, &resp) == -1) {
                    resp.error = true;
                    resp.conn_close = true;
                    logging("response error\n");
                }
            }

            if (resp.isCGI == true && resp.error == false) {
                if (resp.cgiNode == NULL) {
                    logging("Failed to get CGI node\n");
                }
                resp.cgiNode->connNode = cur_node;

                addCGI(resp.cgiNode, p);
            }
            if ((resp.error == true && resp.keepAlive == false) || resp.conn_close == true ) {
                /*close the connection*/
                logging("close connection %d\n", cur_node->connfd);
                close(cur_node->connfd);
                FD_CLR(cur_node->connfd, &p->read_set);

                /*remove node from the connection list*/
                tormNode = cur_node;
                cur_node = cur_node->next;
                remove_node(tormNode, p);
                p->ndp--;
            }

            if (resp.path) {
                free(resp.path);
            }

            if (resp.hdhead) {
                freelist(resp.hdhead);
            }
            if (resp.postbody) {
                free(resp.postbody);
            }
            logging("-------------------connection handling finished----------------------\n");
            continue;
        }
//        logging("At the end of http handling loop\n");
        cur_node = cur_node->next;
    }

    while (cur_cgi != NULL && p->nconn > 0) {
//        logging("Now in the cgi handling loop\n");
        if(FD_ISSET(cur_cgi->connfd, &p->ready_set)){
            cgi_node* tormNode = NULL;
            logging("--------------Now start handling cgi output at %d-----------------\n", cur_cgi->connfd);
            p->nconn--;
            if (CGIresp(cur_cgi) == -1) {
                logging("CGI response error\n");
            }

            tormNode = cur_cgi;
            cur_cgi = cur_cgi->next;
            FD_CLR(tormNode->connfd, &p->read_set);
            removeCGI(tormNode, p);
            p->ndp--;
            logging("-------------Finish the cgi output handling-------------------\n");
            continue;
        }
//        logging("At the end of cgi handling loop\n");
        cur_cgi = cur_cgi->next;
    }
}

int add_conn(int connfd, pool *p, struct sockaddr_in* cli_addr) {
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
    new_node->addr = inet_ntoa(cli_addr->sin_addr);
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

void handleErr(int erro) {
    switch (erro) {
        case SSL_ERROR_NONE:
            logging("SSL_ERROR_NONE\n");
            break;
        case SSL_ERROR_ZERO_RETURN:
            logging("SSL_ERROR_ZERO_RETURN\n");
            break;
        case SSL_ERROR_WANT_READ:
            logging("SSL_ERROR_WANT_READ\n");
            break;
        case SSL_ERROR_WANT_CONNECT:
            logging("SSL_ERROR_WANT_CONNECT\n");
            break;
        case SSL_ERROR_WANT_X509_LOOKUP:
            logging("SSL_ERROR_WANT_X509_LOOKUP\n");
            break;
        case SSL_ERROR_SYSCALL:
            logging("SSL_ERROR_SYSCALL\n");
            break;
        case SSL_ERROR_SSL:
            logging("SSL_ERROR_SSL\n");
            break;
        default:
            logging("Unkown Error\n");
            break;
    }
}

int add_ssl(int connfd, pool *p, SSL_CTX *ssl_context, struct sockaddr_in* cli_addr) {
    SSL *client_context;
    p->nconn--;
    int ret;

    if (p->ndp == FD_SETSIZE) {
        logging("add_conn error: Too many clients");
        return -1;
    }

    p->ndp++;

    if (p->maxfd < connfd) {
        p->maxfd = connfd;
    }

    if ((client_context = SSL_new(ssl_context)) == NULL) {
//        ERR_print_errors_fp(stderr);
        logging("Error creating client SSL context.\n");
        return -1;
    }

    if (SSL_set_fd(client_context, connfd) == 0) {
        SSL_free(client_context);
        logging("Error creating client SSL context.\n");
        return -1;
    }

    if ((ret = SSL_accept(client_context) <= 0)) {
        SSL_free(client_context);
//        ERR_print_errors_fp(stderr);
        handleErr(ret);
        logging("Error accepting (handshake) client SSL context.\n");
        return -1;
    }

    FD_SET(connfd, &p->read_set);
    conn_node *new_node = malloc(sizeof(conn_node));
    new_node->connfd = connfd;
    new_node->isSSL = true;
    new_node->context = client_context;
    new_node->addr = inet_ntoa(cli_addr->sin_addr);
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


static void addCGI(cgi_node *node, pool* p){
    int connfd = node->connfd;
    if (p->ndp == FD_SETSIZE) {
        logging("add_conn error: Too many clients");
        return;
    }

    p->ndp++;

    if (p->maxfd < connfd) {
        p->maxfd = connfd;
    }

    FD_SET(connfd, &p->read_set);
    cgi_node *new_node = node;
    new_node->next = NULL;
    new_node->prev = NULL;

    if (p->cgi_head == NULL) {
        p->cgi_head = new_node;
        p->cgi_tail = new_node;
    }
    else {
        p->cgi_tail->next = new_node;
        new_node->prev = p->cgi_tail;
        p->cgi_tail = new_node;
    }

    return;

}

static void removeCGI(cgi_node *node, pool* p) {
    if (node == NULL) {
        logging("removeCGI function received a NULL pointer\n");
        return;
    }
    if (node->prev != NULL) {
        node->prev->next = node->next;
    }
    else{
        p->cgi_head = node->next;
        if (node->next != NULL) {
            node->next->prev = NULL;
        }
    }

    if (node->next != NULL) {
        node->next->prev = node->prev;
    }
    else{
        p->cgi_tail = node->prev;
        if (node->prev != NULL) {
            node->prev->next = NULL;
        }
    }

    free(node);
}