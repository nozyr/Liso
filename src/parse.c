#include "parse.h"


static int parseUri(char *uri, char *page);

char *getValueByKey(hdNode *head, char *key) {
    hdNode *curNode = head;


    if (head == NULL) {
        logging("The getValueByKey receive a null head!\n");
        return NULL;
    }

//    logging("Start looking for key:%s\n", key);
    while (curNode != NULL) {
        if (strcmp(key, curNode->key) == 0) {
//            logging("Key value found %s\n", curNode->value);
            return curNode->value;
        }
        curNode = curNode->next;
    }
//    logging("Key value not found!\n");
    return NULL;
}

static int readline(conn_node *node, char *buf, int length) {
    if (node->isSSL == true) {
        return sslreadline(node->context, buf, length);
    }
    else {
        return httpreadline(node->connfd, buf, length);
    }
}

static int readblock(conn_node *node, char *buf, int length) {
    if (node->isSSL == true) {
        int i;
        for (i = 1; i <= length; i++) {
            char c;
            ssize_t rc;
            if ((rc = SSL_read(node->context, &c, 1)) == 1) {
                *buf++ = c;
            }
            else if (rc == 0) {
                if (i == 1) {
                    return 0;
                }
                else {
                    break;
                }
            }
            else {
                return -1;
            }
        }


        return i - 1;
    }
    else {
        return (int) read(node->connfd, buf, length);
    }
}

hdNode *newNode(char *key, char *value) {
    hdNode *node = malloc(sizeof(hdNode));
    node->key = key;
    node->value = value;
    node->prev = NULL;
    node->next = NULL;

    return node;
}

void inserthdNode(response_t *resp, hdNode *newNode) {
    if (resp->hdhead == NULL) {
        resp->hdhead = newNode;
        resp->hdtail = newNode;
    }
    else {
        resp->hdtail->next = newNode;
        newNode->prev = resp->hdtail;
        resp->hdtail = newNode;
        resp->hdtail->next = NULL;
    }
}

static bool isCGIreq(char *uri) {
    if (strstr(uri, "/cgi/") == uri) {
        return true;
    }

    return false;
}

int parseRequest(conn_node *node, response_t *resp) {
    char buf[BUFSIZE], method[BUFSIZE], version[BUFSIZE];
    char *connection = NULL;
    int n, post_len = -1, ret = 0;
    bool isPost = false;

    /*Read the request line*/
    if ((n = readline(node, buf, BUFSIZE)) <= 0) {
        if (n == -1) {
            resp->error = true;
            resp->status = INTERNAL_SERVER_ERROR;
            resp->conn_close = true;
            logging("Can not read from socket %d\n", node->connfd);
            logging("%s", strerror(errno));
            return -1;
        }
        if (n == 0) {
            logging("The EOF condition is triggered\n");
            resp->error = true;
            resp->conn_close = true;
            return 0;
        }
    }


    logging("The request status is %s\n", buf);

    if (sscanf(buf, "%s %s %s", method, resp->uri, version) < 3) {
        resp->error = true;
        resp->status = BAD_REQUEST;
//        resp->conn_close = true;
        ret = -1;
    }

    if (resp->error == false && !strstr(version, "HTTP/1.1")) {
        logging("Http version not supported! Stop parsing!!\n");
        resp->error = true;
        resp->status = HTTP_VERSION_NOT_SUPPORTED;
        ret = -1;
    }

    logging("Start Parsing uri: %s\n", resp->uri);
    if (resp->error == false && parseUri(resp->uri, resp->page) < 0) {
        resp->error = true;
        resp->status = BAD_REQUEST;
//        resp->conn_close = true;
        ret = -1;
    }

    /*Judge the request is cgi or not*/
    if (resp->error == false) {
        resp->isCGI = isCGIreq(resp->page);

        /*Judge the request method*/
        if (!strcmp(method, "GET")) {
            resp->method = GET;
        }
        else if (!strcmp(method, "HEAD")) {
            resp->method = HEAD;
        }
        else if (!strcmp(method, "POST")) {
            resp->method = POST;
            isPost = true;
        }
        else {
            resp->method = NOT_SUPPORT;
            resp->error = true;
            resp->status = NOT_IMPLEMENTED;
            ret = -1;
        }

    }


    do { /*Read the rest of the headers*/
        char *pos = NULL;
        char *key = NULL;
        char *value = NULL;
        memset(buf, 0, BUFSIZE);
        n = readline(node, buf, BUFSIZE);
        logging("%s", buf);

        if (resp->error == false && n > 2) {
            key = malloc(BUFSIZE);
            value = malloc(BUFSIZE);

            memset(key, 0, BUFSIZE);
            memset(value, 0, BUFSIZE);

            if (sscanf(buf, "%[a-zA-Z0-9-]:%8192[^\r\n]", key, value) != 2) {
                resp->error = true;
                resp->status = BAD_REQUEST;
                logging("parsing header line %s error!\n", buf);
                free(key);
                free(value);
                ret = -1;
            }
            else{
                key = realloc(key, strlen(key) + 1);
                value = realloc(value, strlen(value) + 1);
                inserthdNode(resp, newNode(key, value));
                resp->hdlineNum++;
            }
        }


        if (isPost == true) {
            pos = strstr(buf, "Content-Length");

            if (pos) {
                sscanf(buf, "Content-Length:%d\n", &post_len);
            }
        }
    } while (n > 2);

    logging("finished reading the header\n");

    /*if is post method, read the content in the body*/
    if (isPost == true && post_len > 0) {
        int rc;
        resp->postbody = malloc(post_len * 2);
        memset(resp->postbody, 0, post_len * 2);
        rc = readblock(node, resp->postbody, post_len);
        if (rc != post_len) {
            logging("error! post length %d not equal to post_len\n", rc);
            resp->error = true;
            resp->status = BAD_REQUEST;
            ret = -1;
        }
        resp->postlen = rc;
        logging("The postbody is:\n%s\n", resp->postbody);
        logging("The post length is: %d\n", resp->postlen);
    }
    else if (isPost == true && post_len < 0) {
        resp->error = true;
        resp->status = BAD_REQUEST;
        return -1;
    }

    connection = getValueByKey(resp->hdhead, "Connection");

    if (connection != NULL) {
        if (strstr(connection, "Keep-Alive")) {
            resp->keepAlive = true;
        }
        else if (strstr(connection, "Close")) {
            resp->conn_close = true;
        }

    }

    return ret;
}

static int parseUri(char *uri, char *page) {
    memset(page, 0, BUFSIZE);
    logging("Parsing Uri..............\n");

   if (uri[0] != '/') {
        logging("The uri %s is invalid\n", uri);
        return -1;
    }
    else {
        strcpy(page, uri);
    }

    if (!strcmp(page, "/")) {
        strcat(page, "index.html");
    }


    logging("The uri is %s\n", uri);
    logging("The parsed page is %s\n", page);

    return 0;
}

void responseinit(response_t *resp) {

    resp->method = GET;
    resp->ishttps = false;
    resp->keepAlive = false;
    resp->conn_close = false;
    resp->status = OK;
    resp->content_len = 0;
    resp->error = false;
    resp->isCGI = false;
    resp->path = NULL;
    resp->filetype = OTHER;
    resp->postbody = NULL;
    resp->hdlineNum = 0;
    resp->postlen = 0;
    resp->cgiNode = NULL;
    resp->hdhead = NULL;
    resp->hdtail = NULL;
    resp->envphead = NULL;
    resp->envptail = NULL;
    resp->addr = NULL;
    memset(resp->header, 0, BUFSIZE);
    memset(resp->uri, 0, BUFSIZE);
    memset(resp->page, 0, BUFSIZE);

    return;
}