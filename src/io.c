#include "io.h"

static char *_lock_file;
static char *_www_folder;
static char *_cgi_path;

int initIO(char *lock_file, char *www_folder, char *cgi_path) {
    if (lock_file == NULL || www_folder == NULL || cgi_path == NULL) {
        return -1;
    }

    _lock_file = lock_file;
    _www_folder = www_folder;
    _cgi_path = cgi_path;

    return 0;

}

int httpreadline(int fd, char *buf, int size) {
    int i;

    for (i = 1; i < size; i++) {
        char c;
        ssize_t rc;
        if ((rc = read(fd, &c, 1)) == 1) {
            *buf++ = c;
            if (c == '\n') {
                break;
            }
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

    *buf = 0;
    return i;
}


int sslreadline(SSL* client_context, char *buf, int size) {
    int i;

    for (i = 1; i < size; i++) {
        char c;
        ssize_t rc;
        if ((rc = SSL_read(client_context, &c, 1)) == 1) {
            *buf++ = c;
            if (c == '\n') {
                break;
            }
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

    *buf = 0;
    return i;
}

char *getpath(char *file) {
    char *fullpath;

    if (file == NULL) {
        return NULL;
    }

    fullpath = malloc(strlen(_www_folder) + strlen(file) + 1);
    strcpy(fullpath, _www_folder);
    strcat(fullpath, file);

    return fullpath;

}

