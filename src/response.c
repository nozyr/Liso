#include "response.h"
#include "parse.h"

#define BUFSIZE 8193
void addstatus(char *header, status_t status);
void addfield(response_t *resp, field_t field);
void getFiletype(response_t *resp);
char* getContentType(MIMEType type);

static char* Page_not_found_response =
"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n"
"<html>\n"
"<head>\n"
"<meta http-equiv=\"Content-type\" content=\"text/html;charset=UTF-8\">\n"
"<title>404 - Document not found</title>\n"
"</head>\n"
"<body>\n"
"<h1>Document not found</h1>\n"
"\n"
"<p>The requested address (URL) was not found on this server.</p><p>You may have used an outdated link or may have typed the address incorrectly.</p>\n"
"<hr>\n"
"\n"
"<!-- IE: document must be at least 512 bytes --> \n"
"<!--\n"
"<p>\n"
"Lorem ipsum dolor sit amet, consectetur adipisicing elit,sadfasdfasdf \n"
"sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. \n"
"Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris \n"
"nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in \n"
"reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla \n"
"pariatur. Excepteur sint occaecat cupidatat non proident, sunt in \n"
"culpa qui officia deserunt mollit anim id est laborum.asdfadfsdafsadf\n"
"</p>\n"
"<p>\n"
"Lorem ipsum dolor sit amet, consectetur adipisicing elit,asdfadfsadf \n"
"sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. \n"
"Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris \n"
"nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in \n"
"reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla \n"
"pariatur. Excepteur sint occaecat cupidatat non proident, sunt in \n"
"culpa qui officia deserunt mollit anim id est laboruasdfasdfasdfasm.\n"
"</p>\n"
"-->\n"
"\n"
"</body>\n"
"</html>";

int writecontent(conn_node* node, char* buf, int length) {
    if (node == NULL) {
        logging("Invalid connection node!\n");
        return -1;
    }
    if (node->isSSL == true) {
        return SSL_write(node->context, buf, length);
    }
    else {
        return (int) write(node->connfd, buf, length);
    }
}

void buildheader(response_t *resp) {
    logging("Start Building the header\n");

    addstatus(resp->header, resp->status);

    logging("Adding status finished\n");
    addfield(resp, TIME);
    addfield(resp, CONNECTION_ALIVE);
    addfield(resp, SERVER);
    addfield(resp, CONTENT_TYP);
    addfield(resp, CONTENT_LEN);
    addfield(resp, LAST_MDY);

    strcat(resp->header, "\r\n");

    return;
}

int buildresp(conn_node* node, response_t *resp) {
    struct stat fileStat;
//    logging("The header length now is %d\n", strlen(resp->header));

    /*if error exist, send error message back*/
    if (resp->error == true) {
        logging("message error, send connection close\n");
        addstatus(resp->header, resp->status);
        addfield(resp, CONNECTION_CLOSE);
        if(writecontent(node, resp->header, strlen(resp->header)) <= 0){
            logging("Writing refusion message failed.\n");
        }
        return 0;
    }

    if (resp->isCGI == true) {
        logging("Entering CGI response building routine\n");
        return cgihandle(resp);
    }
    resp->path = getpath(resp->page);

    logging("The requested file is located at %s\n", resp->path);

    /*if file not exists, send 404 message back*/
    if (stat(resp->path, &fileStat) == -1) {
        resp->status = NOT_FOUND;
        resp->content_len = (off_t)strlen(Page_not_found_response);
        resp->filetype = HTML;
        buildheader(resp);
        writecontent(node, resp->header, strlen(resp->header));
        writecontent(node, Page_not_found_response, strlen(Page_not_found_response));
        logging("file %s don't exists\n", resp->path);
        return 0;
    }

    /*if file path is valid, get the file metadata*/
    resp->content_len = fileStat.st_size;
    resp->last_md = fileStat.st_mtime;
    getFiletype(resp);

    /*Build file header*/
    buildheader(resp);

    logging("Header building finished, sending header back\n");

    /*Sending file header back*/
    if (writecontent(node, resp->header, strlen(resp->header)) <= 0) {
        logging("Writing header to socket %d failed\n", node->connfd);
    }

    logging("Header Sending Finished\n");

    /*if method not head, send message body back*/
    if (resp->method != HEAD) {
        logging("Now Sending the Content\n");
        char *content = malloc(resp->content_len);

        int pagefd = open(resp->path, O_RDONLY);
        if (read(pagefd, content, resp->content_len) <= 0) {
            logging("Reading content from file %s failed\n", resp->path);
            close(pagefd);
            return 0;
        }
        logging("Reading content from file %s Successed\n", resp->path);

        if (writecontent(node, content, resp->content_len) <= 0){
            logging("Writing header to socket %d failed\n", node->connfd);
        }
        close(pagefd);
        free(content);
    }

    logging("Responding finished\n");
    return 0;
}

void addstatus(char *header, status_t status) {
    char *statusl;
    if (header == NULL) {
        logging("Header pointer is NULL!! Error!!\n");
    }

    logging("Adding status line now.\n");
    switch (status) {
        case OK:
            statusl = "HTTP/1.1 200 OK\r\n";
            break;
        case BAD_REQUEST:
            statusl = "HTTP/1.1 400 BAD REQUEST\r\n";
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
    logging("%s", statusl);
    strcat(header, statusl);
}

void addfield(response_t *resp, field_t field) {
    char *fieldl = NULL;
    char buf[BUFSIZE];
    time_t current_time;
    struct tm* cur_gmtime = NULL;
    memset(buf, 0, BUFSIZE);

//    logging("Start Adding field\n");

    switch (field) {
        case CONNECTION_CLOSE:
//            logging("1\n");
            fieldl = "connection: close\r\n";
            break;
        case CONNECTION_ALIVE:
//            logging("2\n");
            fieldl = "connection: Keep-Alive\r\nKeep_Alive: Test\r\n";
            break;
        case TIME:
//            logging("3\n");
            cur_gmtime = gmtime(&current_time);
            if (cur_gmtime == NULL) {
                logging("Getting current time error!\n");
                return;
            }
            strftime(buf, BUFSIZE, "Date: %a, %d %b %Y %H:%M:%S %Z\r\n", cur_gmtime);
            fieldl = buf;
            break;
        case SERVER:
//            logging("4\n");
            fieldl = "server: Liso/1.0\r\n";
            break;
        case CONTENT_LEN:
//            logging("5\n");
            sprintf(buf, "Content-Length: %d\r\n", (int)resp->content_len);
            fieldl = buf;
            break;
        case CONTENT_TYP:
//            logging("6\n");
            sprintf(buf, "Content-Type: %s\r\n", getContentType(resp->filetype));
            fieldl = buf;
            break;
        case LAST_MDY:
//            logging("7\n");
            cur_gmtime = gmtime(&resp->last_md);
            strftime(buf, 256, "Last-Modified: %a, %d %b %Y %H:%M:%S %Z\r\n", cur_gmtime);
            fieldl = buf;
            break;
        default:
            logging("error! Unkonw filed status!\n");
            break;
    }

    logging("%s", fieldl);
    strcat(resp->header, fieldl);
}

void getFiletype(response_t *resp) {

    if (strlen(resp->path) < 4) {
        resp->filetype = OTHER;
    }
    else {
        char *ext = strrchr(resp->path, '.');

        if (ext == NULL) {resp->filetype = OTHER;}
        else if (strcmp(ext, ".html") == 0) {resp->filetype = HTML;}
        else if (strcmp(ext, ".htm") == 0) {resp->filetype = HTML;}
        else if (strcmp(ext, ".css") == 0) {resp->filetype = CSS;}
        else if (strcmp(ext, ".png") == 0) {resp->filetype =  PNG;}
        else if (strcmp(ext, ".jpeg") == 0) {resp->filetype = JPEG;}
        else if (strcmp(ext, ".gif") == 0) {resp->filetype = GIF;}
    }
    return;
}

char* getContentType(MIMEType type)
{
    switch(type) {
        case HTML:
            return "text/html";
        case CSS:
            return "text/css";
        case JPEG:
            return "image/jpeg";
        case PNG:
            return "image/png";
        case GIF:
            return "image/gif";
        default:
            return "application/octet-stream";
    }
}