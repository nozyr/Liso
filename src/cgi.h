#ifndef CGI_H
#define CGI_H

#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <sys/stat.h>

#include "log.h"
#include "parse.h"
#include "response.h"

int initCGI(char *cgipath, char* http_port, char* https_port);
int cgihandle(response_t* resp);
int CGIresp(cgi_node *node);
#endif