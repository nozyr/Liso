
#include "parse.h"

#define BUFSIZE 8096

void parse(int connfd)
{
    char buf[BUFSIZE];

    if (read(connfd, buf, BUFSIZE) == 0)
    {
        //todo send error message
        return;
    }

}