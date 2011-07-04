#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <openssl/err.h>
#include <zmq.h>

#include "state_machine.h"

int main(int argc,char **argv)
    {
    SSLStateMachine *pMachine;
    int nPort;
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

    void* context = zmq_init(1);
    void* zmqSocket = zmq_socket(context, ZMQ_PAIR);
    char portstr[128];
    snprintf(portstr, sizeof(portstr), "tcp://*:%i", nPort);
    zmq_bind(zmqSocket, portstr);

    pMachine=SSLStateMachine_new(szCertificateFile, szKeyFile, 1);

    StateMachineEchoLoop(pMachine, zmqSocket);

    SSL_free(pMachine->pSSL);
    SSL_CTX_free(pMachine->pCtx);
    free(pMachine);
    zmq_close(zmqSocket);
    zmq_term(context);

    return 0;
    }
