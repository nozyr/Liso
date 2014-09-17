#ifndef REPONSE_H
#define REPONSE_H

#include "io.h"
#include "string.h"
#include "log.h"
#include "parse.h"
#include <sys/stat.h>

void pagersp(int connfd, char *page);

void addstatus(char *header, status_t status);

void addfield(char *header, field_t field);

int response(int connfd, response_t response);

#endif