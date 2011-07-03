#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <openssl/err.h>

#include "state_machine.h"

int OpenSocket(int nPort)
    {
    int nSocket;
    struct sockaddr_in saServer;
    struct sockaddr_in saClient;
    int one=1;
    int nSize;
    int nFD;
    socklen_t nLen;

    nSocket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(nSocket < 0)
    {
    perror("socket");
    exit(1);
    }

    if(setsockopt(nSocket,SOL_SOCKET,SO_REUSEADDR,(char *)&one,sizeof one) < 0)
    {
    perror("setsockopt");
        exit(2);
    }

    memset(&saServer,0,sizeof saServer);
    saServer.sin_family=AF_INET;
    saServer.sin_port=htons(nPort);
    nSize=sizeof saServer;
    if(bind(nSocket,(struct sockaddr *)&saServer,nSize) < 0)
    {
    perror("bind");
    exit(3);
    }

    if(listen(nSocket,512) < 0)
    {
    perror("listen");
    exit(4);
    }

    nLen=sizeof saClient;
    nFD=accept(nSocket,(struct sockaddr *)&saClient,&nLen);
    if(nFD < 0)
    {
    perror("accept");
    exit(5);
    }

    fprintf(stderr,"Incoming accepted on port %d\n",nPort);

    return nFD;
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

    pMachine=SSLStateMachine_new(szCertificateFile, szKeyFile, 1);

    StateMachineEchoLoop(pMachine, nFD);

    SSL_free(pMachine->pSSL);
    SSL_CTX_free(pMachine->pCtx);
    free(pMachine);
    close(nFD);

    return 0;
    }
