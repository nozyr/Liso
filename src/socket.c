#include "socket.h"


int close_socket(int sock) {
    if (close(sock)) {
        logging("Failed closing socket.\n");
        return 1;
    }

    return 0;
}