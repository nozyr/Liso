#include "cgi.h"

#define BUF_SIZE 4096
#define CGI_HEADER_LEN 22

static char * _cgipath;
static char * _http_port;
static char * _https_port;

static char* ARGV[2] = {};


int initCGI(char *cgipath, char* http_port, char* https_port) {
    struct stat fileStat;
    _cgipath = cgipath;
    _http_port = http_port;
    _https_port = https_port;
    ARGV[0] = _cgipath;
    ARGV[1] = NULL;

    if (stat(_cgipath, &fileStat) == -1) {
        logging("The designated cgi file not existed\n");
        return -1;
    }

    return 0;
}

void execve_error_handler() {
    switch (errno) {
        case E2BIG:
            logging("The total number of bytes in the environment "
                    "(envp) and argument list (argv) is too large.\n");
            return;
        case EACCES:
            logging("Execute permission is denied for the file or a script or ELF interpreter.\n");
            return;
        case EFAULT:
            logging("filename points outside your accessible address space.\n");
            return;
        case EINVAL:
            logging("An ELF executable had more than one PT_INTERP segment "
                    "(i.e., tried to name more than one interpreter).\n");
            return;
        case EIO:
            logging("An I/O error occurred.\n");
            return;
        case EISDIR:
            logging("An ELF interpreter was a directory.\n");
            return;
        case ELOOP:
            logging("Too many symbolic links were encountered in resolving "
                    "filename or the name of a script or ELF interpreter.\n");
            return;
        case EMFILE:
            logging("The process has the maximum number of files open.\n");
            return;
        case ENAMETOOLONG:
            logging("filename is too long.\n");
            return;
        case ENFILE:
            logging("The system limit on the total number of open files has been reached.\n");
            return;
        case ENOENT:
            logging("The file filename or a script or ELF interpreter does not exist, "
                    "or a shared library needed for file or interpreter cannot be found.\n");
            return;
        case ENOEXEC:
            logging("An executable is not in a recognised format, is for the wrong architecture, "
                    "or has some other format error that means it cannot be executed.\n");
            return;
        case ENOMEM:
            logging("Insufficient kernel memory was available.\n");
            return;
        case ENOTDIR:
            logging("A component of the path prefix of filename or a script "
                    "or ELF interpreter is not a directory.\n");
            return;
        case EPERM:
            logging("The file system is mounted nosuid, the user is not the superuser, "
                    "and the file has an SUID or SGID bit set.\n");
            return;
        case ETXTBSY:
            logging("Executable was open for writing by one or more processes.\n");
            return;
        default:
            logging("Unkown error occurred with execve().\n");
            return;
    }
}

char** fillenv(response_t *resp){
    hdNode* curNode = resp->envphead;
    hdNode* prevNode = curNode;
    char **envp = malloc((CGI_HEADER_LEN + 1) * sizeof(char *));
    int count = 0;
    int envpSize = CGI_HEADER_LEN + 1;
//    logging("Start filling process\n");
    while(curNode != NULL) {
//        logging("count %d\n", count);
        if (curNode->value[0] == ' '){
            curNode->value++;
        }
        size_t lineSize = strlen(curNode->key) + strlen(curNode->value) + 2;
        char *newLine = malloc(lineSize);
        memset(newLine, 0, lineSize);
        strcpy(newLine, curNode->key);
        strcat(newLine, "=");
        strcat(newLine, curNode->value);
        if (count < envpSize) {
            envp[count] = newLine;
            count++;
        }
        else {
            logging("header size counting error\n");
            return NULL;
        }
        curNode = curNode->next;
        free(prevNode);
        prevNode = curNode;
    }

    logging("the envp entry count is now %d\n", count);
    envp[count] = NULL;
    resp->hdhead = NULL;
    resp->hdtail = NULL;

    return envp;
}

void insertEnvNode(response_t *resp, hdNode *newNode){
    if (resp->envphead == NULL) {
        resp->envphead = newNode;
        resp->envptail = newNode;
    }
    else {
        resp->envptail->next = newNode;
        newNode->prev = resp->envptail;
        resp->envptail = newNode;
        resp->envptail->next = NULL;
    }
}

void insertEnvp(response_t* resp, char* key, char *value) {
//    logging("inserting key: %s\n", key);

    if (value == NULL) {
//        logging("Key %s has NULL value\n");
        insertEnvNode(resp, newNode(key, " "));
    }
    else {
        insertEnvNode(resp, newNode(key, value));
    }
}

static char* methodtoString(method_t method) {
    switch (method) {
        case GET:
            return "GET";
        case HEAD:
            return "HEAD";
        case POST:
            return "POST";
        default:
            return NULL;
    }
    return NULL;
}

static char* getValueByKey(hdNode* head, char *key){
    hdNode* curNode = head;


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


static void buildEnvp(response_t* resp){
    char* uri = resp->uri;
    char *page_end = strchr(uri, '?');
    char *path_info = uri + strlen("/cgi");

    char* query = NULL;

    if (page_end != NULL) {
        *page_end = '\0';
        query = page_end + 1;
    }

    logging("Start inserting\n");
    insertEnvp(resp, "GATEWAY_INTERFACE", "CGI/1.1");
    insertEnvp(resp, "SERVER_PROTOCOL", "HTTP/1.1");
    insertEnvp(resp, "SERVER_SOFTWARE", "Liso/1.0");
    insertEnvp(resp, "SCRIPT_NAME", "/cgi");
//    logging("Static message inserting finished\n");

    insertEnvp(resp, "REQUEST_URI", resp->page);
    insertEnvp(resp, "QUERY_STRING", query);
    insertEnvp(resp, "PATH_INFO", path_info);
    insertEnvp(resp, "REQUEST_METHOD", methodtoString(resp->method));
//    logging("Query inserting finished\n");
    if (resp->ishttps) {
        insertEnvp(resp, "SERVER_PORT", _https_port);
        insertEnvp(resp, "HTTPS", "1");
    }
    else {
        insertEnvp(resp, "SERVER_PORT", _http_port);
        insertEnvp(resp, "HTTP", "1");
    }

    insertEnvp(resp, "REMOTE_ADDR", resp->addr);
    insertEnvp(resp, "CONTENT_LENGTH", getValueByKey(resp->hdhead, "Content-Length"));
    insertEnvp(resp, "CONTENT_TYPE", getValueByKey(resp->hdhead, "Content-Type"));
    insertEnvp(resp, "HTTP_ACCEPT", getValueByKey(resp->hdhead, "Accept"));
    insertEnvp(resp, "HTTP_REFERER", getValueByKey(resp->hdhead, "Referer"));
    insertEnvp(resp, "HTTP_ACCEPT_ENCODING", getValueByKey(resp->hdhead, "Accept-Encoding"));
    insertEnvp(resp, "HTTP_ACCEPT_LANGUAGE", getValueByKey(resp->hdhead, "Accept-Language"));
    insertEnvp(resp, "HTTP_ACCEPT_CHARSET", getValueByKey(resp->hdhead, "Accept-Charset"));
    insertEnvp(resp, "HTTP_HOST", getValueByKey(resp->hdhead, "Host"));
    insertEnvp(resp, "HTTP_COOKIE", getValueByKey(resp->hdhead, "Cookie"));
    insertEnvp(resp, "HTTP_USER_AGENT", getValueByKey(resp->hdhead, "User-Agent"));
    insertEnvp(resp, "HTTP_CONNECTION", getValueByKey(resp->hdhead, "Connection"));
    logging("Inserting Finished\n");

    return;

}

int cgihandle(response_t *resp) {

    /*************** BEGIN VARIABLE DECLARATIONS **************/
    pid_t pid;
    int stdin_pipe[2];
    int stdout_pipe[2];
    char buf[BUF_SIZE];
    int readret;
    char** envp;
    /*************** END VARIABLE DECLARATIONS **************/

    logging("now in cgi handling routine\n");
    /*************** BEGIN PIPE **************/
    /* 0 can be read from, 1 can be written to */
    if (pipe(stdin_pipe) < 0) {
        logging("Error piping for stdin.\n");
        return -1;
    }

    if (pipe(stdout_pipe) < 0) {
        logging("Error piping for stdout.\n");
        return -1;
    }
    /*************** END PIPE **************/

    logging("start building the environment array\n");
    buildEnvp(resp);

    logging("Start Filling envrionment array\n");
    if ((envp = fillenv(resp)) == NULL) {
        return -1;
    }

    logging("Filling environment array successed!\n");
    /*************** BEGIN FORK **************/
    pid = fork();
    /* not good */
    if (pid < 0) {
        logging("Something really bad happened when forking.\n");
        return -1;
    }

    /* child, setup environment, execve */
    if (pid == 0) {
        /*************** BEGIN EXECVE ****************/
        close(stdout_pipe[0]);
        close(stdin_pipe[1]);
        dup2(stdout_pipe[1], fileno(stdout));
        dup2(stdin_pipe[0], fileno(stdin));
        dup2(getlogfd(), fileno(stderr));

        /* pretty much no matter what, if it returns bad things happened... */

        if (execve(_cgipath, ARGV, envp)) {
            execve_error_handler();
            logging("Error executing execve syscall.\n");
            exit(EXIT_FAILURE);
        }
        /*************** END EXECVE ****************/
    }

    if (pid > 0) {
        logging("Parent: Heading to select() loop.\n");
        close(stdout_pipe[1]);
        close(stdin_pipe[0]);

        int i = 0;
        for (i = 0; i < CGI_HEADER_LEN; ++i) {
            free(envp[i]);
        }

        if (resp->method == POST) {
            if (write(stdin_pipe[1], resp->postbody, resp->postlen) < 0) {
                logging( "Error writing to spawned CGI program.\n");
                close(stdin_pipe[1]);
                return -1;
            }
        }
        close(stdin_pipe[1]); /* finished writing to spawn */

        resp->cgiNode = malloc(sizeof(cgi_node));
        resp->cgiNode->connfd = stdout_pipe[0];
        resp->cgiNode->pid = pid;
    }
    /*************** END FORK **************/

    return 0;
}


int CGIresp(cgi_node *node){
    ssize_t readret = 0;
    int readfd = node->connfd;
    char buf[BUFSIZE];

    logging("Now start reading cgi response content\n");
    while ((readret = read(readfd, buf, BUF_SIZE - 1)) > 0) {
        writecontent(node->connNode, buf, readret);
        logging("Got from CGI: %s\n", buf);
    }

    close(readfd);

    if (readret == 0) {
        logging("CGI spawned process returned with EOF as expected.\n");
        return 0;
    }

    logging("Process exiting, badly...how did we get here!?\n");
    return -1;
}