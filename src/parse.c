#include <Python/Python.h>
#include "parse.h"


static int parseUri(char *uri, char *page);

int parseRequest(int connfd, response_t header) {
    char buf[BUFSIZE], method[BUFSIZE], version[BUFSIZE];
    int n, post_len = 0;
    bool isPost = false;

    if (readline(connfd, buf, BUFSIZE) <= 0) {
        header.error = true;
        header.status = INTERNAL_SERVER_ERROR;
        logging("Can not read from socket %d\n", connfd);
        return -1;
    }

    sscanf(buf, "%s %s %s", method, header.uri, version);

    if (!strcmp(version, "HTTP/1.1")) {
        header.version = false;
        header.error = true;
        header.status = HTTP_VERSION_NOT_SUPPORTED;
        return 0;
    }

    header.version = true;

    if (strcmp(method, "GET")) {
        header.method = GET;
    }
    else if (strcmp(method, "HEAD")) {
        header.method = HEAD;
    }
    else if (strcmp(method, "POST")) {
        header.method = POST;
        isPost = true;
    }
    else {
        header.method = NOT_SUPPORT;
    }

    /*Read the rest of the headers*/
    do {
        char* pos = NULL;
        n = readline(connfd, buf, BUFSIZE);
        if (isPost) {
            pos = strstr(buf, "Content-Length");

            if (pos) {
                sscanf(buf, "Content-Length:%d\n", post_len);
            }
        }
    } while (n > 1);

    /*if is post method, read the content in the body*/
    if (isPost && post_len > 0) {
        read(connfd, buf, post_len);
    }

    return 0;
}

static int parseUri(char *uri, char *page) {
    memset(page, 0, BUFSIZE);

    if (strstr(uri, "http://")) {
        sscanf(uri, "http://%*8192[^/]/%8192[^\n]", page);
    }
    else {
        strcpy(page, uri);
    }

    return 0;
}

void responseinit(response_t header) {

    header.method = GET;
    header.status = OK;
    header.error = false;
    header.version = true;
    memset(header.header, 0, BUFSIZE);
    memset(header.uri, 0, BUFSIZE);

    return;
}