#include "https.h"


SSL_CTX* sslinit(char *key, char *crt) {
    SSL_CTX *ssl_context = NULL;
    SSL_load_error_strings();
    SSL_library_init();

    /* we want to use TLSv1 only */
    if ((ssl_context = SSL_CTX_new(SSLv23_server_method())) == NULL) {
        logging("Error creating SSL context.\n");
        return NULL;
    }

    /* register private key */
    if (SSL_CTX_use_PrivateKey_file(ssl_context, key, SSL_FILETYPE_PEM) == 0) {
        SSL_CTX_free(ssl_context);
        logging("Error associating private key.\n");
        return NULL;
    }

    /* register public key (certificate) */
    if (SSL_CTX_use_certificate_file(ssl_context, crt, SSL_FILETYPE_PEM) == 0) {
        SSL_CTX_free(ssl_context);
        logging("Error associating certificate.\n");
        return NULL;
    }

    return ssl_context;
}