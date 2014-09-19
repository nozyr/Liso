#include "parse.h"


static int parseUri(char *uri, char *page);

int parseRequest(int connfd, response_t *resp) {
    char buf[BUFSIZE], method[BUFSIZE], version[BUFSIZE];
    int n, post_len = -1;
    bool isPost = false;

    if ((n = readline(connfd, buf, BUFSIZE)) <= 0) {
        if(n == -1)
        {
            resp->error = true;
            resp->status = INTERNAL_SERVER_ERROR;
            logging("Can not read from socket %d\n", connfd);
            logging("%s", strerror(errno));
            return -1;
        }
        if(n == 0)
        {
            resp->error = true;
            return 0;
        }
    }

    logging("The request status is %s\n", buf);
    sscanf(buf, "%s %s %s", method, resp->uri, version);

    if (!strstr(version, "HTTP/1.1")) {
        logging("Http version not supported! Stop parsing!!\n");
        resp->version = false;
        resp->error = true;
        resp->status = HTTP_VERSION_NOT_SUPPORTED;
        return -1;
    }

    resp->version = true;

    logging("Start Parsing uri: %s\n", resp->uri);
    parseUri(resp->uri, resp->page);

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
        return -1;
    }

    /*Read the rest of the headers*/
    do {
        char *pos = NULL;
        n = readline(connfd, buf, BUFSIZE);
        logging("%s", buf);
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
        read(connfd, buf, post_len);
    }
    else if (isPost == true && post_len == -1) {
        resp->error = true;
        resp->status = LENGTH_REQUIRED;
        return -1;
    }

    return 1;
}

static int parseUri(char *uri, char *page) {
    memset(page, 0, BUFSIZE);
    int status, temport;
    char host[BUFSIZE];
    logging("Parsing Uri..............\n");

    if (strstr(uri, "http://")) {
        if (sscanf(uri, "http://%8192[^:]:%i/%8192[^\n]", host, &temport, page) == 3) {status = 1;}
        else if (sscanf(uri, "http://%8192[^/]/%8192[^\n]", host, page) == 2) {status = 2;}
        else if (sscanf(uri, "http://%8192[^:]:%i[^\n]", host, &temport) == 2) {status = 3;}
        else if (sscanf(uri, "http://%8192[^/]", host) == 1) {status = 4;}
    }
    else if (!strstr(uri, "/")) {
        sprintf(page, "/");
    }
    else {
        strcpy(page, uri);
    }

    if (status > 3) {
        sprintf(page, "/index.html");
    }
    else if (!strcmp(page, "/")) {
        strcat(page, "index.html");
    }


    logging("The uri is %s\n", uri);
    logging("The parsed page is %s\n", page);

    return 0;
}

void responseinit(response_t *resp) {

    resp->method = GET;
    resp->status = OK;
    resp->content_len = 0;
    resp->error = false;
    resp->version = true;
    resp->path = NULL;
    resp->filetype = OTHER;
    memset(resp->header, 0, BUFSIZE);
    memset(resp->uri, 0, BUFSIZE);
    memset(resp->page, 0, BUFSIZE);

    return;
}