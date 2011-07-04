#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <openssl/ssl.h>
#include <assert.h>

/* die_unless is intended to work like assert, except that it happens
 always, even if NDEBUG is defined. Use assert as a stopgap. */

#define die_unless(x) assert(x)

typedef struct
{
    SSL_CTX *pCtx;
    BIO *pbioRead;
    BIO *pbioWrite;
    SSL *pSSL;
} SSLStateMachine;

void SSLStateMachine_print_error(SSLStateMachine *pMachine, const char *szErr);

SSLStateMachine *SSLStateMachine_new(const char *szCertificateFile,
                                     const char *szKeyFile, int isServer);

void SSLStateMachine_read_inject(SSLStateMachine *pMachine,
                                 const unsigned char *aucBuf, int nBuf);

int SSLStateMachine_read_extract(SSLStateMachine *pMachine,
                                 unsigned char *aucBuf, int nBuf);

int SSLStateMachine_write_can_extract(SSLStateMachine *pMachine);

int SSLStateMachine_write_extract(SSLStateMachine *pMachine,
                                  unsigned char *aucBuf, int nBuf);

void SSLStateMachine_write_inject(SSLStateMachine *pMachine,
                                  const unsigned char *aucBuf, int nBuf);

void StateMachineEchoLoop(SSLStateMachine *pMachine, void* zmqSocket);

#endif
