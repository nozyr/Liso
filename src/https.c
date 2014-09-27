#include "https.h"



int sslinit(SSL_CTX* ssl_context, char* private, char* crt){
    SSL_load_error_strings();
    SSL_library_init();

    /* we want to use TLSv1 only */
    if ((ssl_context = SSL_CTX_new(TLSv1_server_method())) == NULL)
    {
        logging("Error creating SSL context.\n");
        return -1;
    }

    /* register private key */
    if (SSL_CTX_use_PrivateKey_file(ssl_context, private,
            SSL_FILETYPE_PEM) == 0)
    {
        SSL_CTX_free(ssl_context);
        logging("Error associating private key.\n");
        return -1;
    }

    /* register public key (certificate) */
    if (SSL_CTX_use_certificate_file(ssl_context, crt,
            SSL_FILETYPE_PEM) == 0)
    {
        SSL_CTX_free(ssl_context);
        logging("Error associating certificate.\n");
        return -1;
    }

    return 0;
}