#include "response.h"

#define BUFSIZE 8193

void pagersp(int connfd, char *page) {
    struct stat fileStat;
    char *file_path;
    char header[BUFSIZE];
    if (strcmp(page, "/")) {
        file_path = getpath("index.html");
    }
    else {
        file_path = getpath(page);
    }

    logging("Open File at path %s", file_path);
    if (stat(file_path, &fileStat) == -1) {
        free(file_path);
        addstatus(header, NOT_FOUND);
        return;
    }


    return;
}

int response(int connfd, response_t header) {

    if (header.error) {
        addstatus(header.header, header.status);
        addfield(header.header, CONNECTION_CLOSE);
        write(connfd, header.header, strlen(header.header));
    }

    return 0;
}

void addstatus(char *header, status_t status) {
    char *statusl;

    logging("Adding status line now.\n");
    switch (status) {
        case OK:
            statusl = "HTTP/1.1 200 OK\r\n";
            break;
        case NOT_FOUND:
            statusl = "HTTP/1.1 404 NOT FOUND\r\n";
            break;
        case LENGTH_REQUIRED:
            statusl = "HTTP/1.1 411 LENGTH REQUIRED\r\n";
            break;
        case INTERNAL_SERVER_ERROR:
            statusl = "HTTP/1.1 500 INTERNAL SERVER ERROR\r\n";
            break;
        case NOT_IMPLEMENTED:
            statusl = "HTTP/1.1 501 NOT IMPLEMENTED\r\n";
            break;
        case SERVICE_UNAVAILABLE:
            statusl = "HTTP/1.1 503 SERVICE UNAVAILABLE\r\n";
            break;
        case HTTP_VERSION_NOT_SUPPORTED:
            statusl = "HTTP/1.1 505 HTTP VERSION NOT SUPPORTED\r\n";
            break;
        default:
            statusl = "HTTP/1.1 500 INTERNAL SERVER ERROR\r\n";
            break;
    }
    logging("Status Line is %s\n", statusl);
    strcat(header, statusl);
}

void addfield(char *header, field_t field) {
    char *fieldl;

    logging("Adding files line now\n");
    switch (field) {
        case CONNECTION_CLOSE:
            fieldl = "connection: close\r\n";
            break;
        case CONNECTION_ALIVE:
            fieldl = "connection: Keep-Alive\r\n";
            break;
        case TIME:
            fieldl = "time: \r\n";
            break;
        case SERVER:
            fieldl = "SERVER: Liso/1.0\r\n";
            break;
        case CONTENT_LEN:
            break;
        case CONTENT_TYP:
            break;
        case LAST_MDY:
            break;
        default:
            break;
    }

    logging("Field line is %s\n", fieldl);
    strcat(header, fieldl);
}