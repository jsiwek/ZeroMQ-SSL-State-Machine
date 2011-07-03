#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/err.h>

#include "state_machine.h"

int OpenSocket(int nPort)
    {
    char port[8];
    int nSocket;
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    snprintf(port, sizeof(port), "%i", nPort);
    getaddrinfo("localhost", port, &hints, &res);

    nSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if ( nSocket < 0 )
        {
        perror("socket");
        exit(1);
        }

    if ( connect(nSocket, res->ai_addr, res->ai_addrlen) )
        {
        perror("connect");
        exit(1);
        }

    freeaddrinfo(res);

    return nSocket;
    }

int main(int argc,char **argv)
    {
    SSLStateMachine *pMachine;
    int nPort;
    int nFD;
    const char *szCertificateFile;
    const char *szKeyFile;

    if(argc != 4)
    {
    fprintf(stderr,"%s <port> <certificate file> <key file>\n",argv[0]);
    exit(6);
    }

    nPort=atoi(argv[1]);
    szCertificateFile=argv[2];
    szKeyFile=argv[3];

    SSL_library_init();
    OpenSSL_add_ssl_algorithms();
    SSL_load_error_strings();
    ERR_load_crypto_strings();

    nFD=OpenSocket(nPort);

    pMachine=SSLStateMachine_new(szCertificateFile, szKeyFile, 0);

    StateMachineEchoLoop(pMachine, nFD);

    SSL_free(pMachine->pSSL);
    SSL_CTX_free(pMachine->pCtx);
    free(pMachine);
    close(nFD);

    return 0;
    }
