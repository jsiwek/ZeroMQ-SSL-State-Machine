/* ====================================================================
 * Copyright (c) 2000 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.openssl.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    openssl-core@openssl.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.openssl.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This product includes cryptographic software written by Eric Young
 * (eay@cryptsoft.com).  This product includes software written by Tim
 * Hudson (tjh@cryptsoft.com).
 *
 */

/*
 * Nuron, a leader in hardware encryption technology, generously
 * sponsored the development of this demo by Ben Laurie.
 *
 * See http://www.nuron.com/.
 */

/*
 * the aim of this demo is to provide a fully working state-machine
 * style SSL implementation, i.e. one where the main loop acquires
 * some data, then converts it from or to SSL by feeding it into the
 * SSL state machine. It then does any I/O required by the state machine
 * and loops.
 *
 * In order to keep things as simple as possible, this implementation
 * listens on a TCP socket, which it expects to get an SSL connection
 * on (for example, from s_client) and from then on writes decrypted
 * data to stdout and encrypts anything arriving on stdin. Verbose
 * commentary is written to stderr.
 *
 * This implementation acts as a server, but it can also be done for a client.  */

#include <openssl/ssl.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <openssl/err.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <zmq.h>

#include "state_machine.h"

void SSLStateMachine_print_error(SSLStateMachine *pMachine,const char *szErr)
    {
    unsigned long l;

    fprintf(stderr,"%s\n",szErr);
    while((l=ERR_get_error()))
	{
	char buf[1024];

	ERR_error_string_n(l,buf,sizeof buf);
	fprintf(stderr,"Error %lx: %s\n",l,buf);
	}
    }

SSLStateMachine *SSLStateMachine_new(const char *szCertificateFile,
				     const char *szKeyFile, int isServer)
    {
    SSLStateMachine *pMachine=malloc(sizeof *pMachine);
    int n;

    die_unless(pMachine);

    if ( isServer )
        pMachine->pCtx=SSL_CTX_new(SSLv23_server_method());
    else
        pMachine->pCtx=SSL_CTX_new(SSLv23_client_method());

    die_unless(pMachine->pCtx);

    n=SSL_CTX_use_certificate_file(pMachine->pCtx,szCertificateFile,
				   SSL_FILETYPE_PEM);
    die_unless(n > 0);

    n=SSL_CTX_use_PrivateKey_file(pMachine->pCtx,szKeyFile,SSL_FILETYPE_PEM);
    die_unless(n > 0);

    pMachine->pSSL=SSL_new(pMachine->pCtx);
    die_unless(pMachine->pSSL);

    pMachine->pbioRead=BIO_new(BIO_s_mem());

    pMachine->pbioWrite=BIO_new(BIO_s_mem());

    SSL_set_bio(pMachine->pSSL,pMachine->pbioRead,pMachine->pbioWrite);

    if ( isServer )
        SSL_set_accept_state(pMachine->pSSL);
    else
        SSL_set_connect_state(pMachine->pSSL);

    return pMachine;
    }

void SSLStateMachine_read_inject(SSLStateMachine *pMachine,
				 const unsigned char *aucBuf,int nBuf)
    {
    int n=BIO_write(pMachine->pbioRead,aucBuf,nBuf);
    /* If it turns out this assert fails, then buffer the data here
     * and just feed it in in churn instead. Seems to me that it
     * should be guaranteed to succeed, though.
     */
    assert(n == nBuf);
    fprintf(stderr,"%d bytes of encrypted data fed to state machine\n",n);
    }

int SSLStateMachine_read_extract(SSLStateMachine *pMachine,
				 unsigned char *aucBuf,int nBuf)
    {
    int n;

    if(!SSL_is_init_finished(pMachine->pSSL))
	{
	fprintf(stderr,"Doing SSL_accept\n");
	n=SSL_accept(pMachine->pSSL);
	if(n == 0)
	    fprintf(stderr,"SSL_accept returned zero\n");
	if(n < 0)
	    {
	    int err;

	    if((err=SSL_get_error(pMachine->pSSL,n)) == SSL_ERROR_WANT_READ)
		{
		fprintf(stderr,"SSL_accept wants more data\n");
		return 0;
		}

	    SSLStateMachine_print_error(pMachine,"SSL_accept error");
	    exit(7);
	    }
	return 0;
	}

    n=SSL_read(pMachine->pSSL,aucBuf,nBuf);
    if(n < 0)
	{
	int err=SSL_get_error(pMachine->pSSL,n);

	if(err == SSL_ERROR_WANT_READ)
	    {
	    fprintf(stderr,"SSL_read wants more data\n");
	    return 0;
	    }

	SSLStateMachine_print_error(pMachine,"SSL_read error");
	exit(8);
	}

    fprintf(stderr,"%d bytes of decrypted data read from state machine\n",n);
    return n;
    }

int SSLStateMachine_write_can_extract(SSLStateMachine *pMachine)
    {
    int n=BIO_pending(pMachine->pbioWrite);
    if(n)
	fprintf(stderr,"There is encrypted data available to write\n");
    else
	fprintf(stderr,"There is no encrypted data available to write\n");

    return n;
    }

int SSLStateMachine_write_extract(SSLStateMachine *pMachine,
				  unsigned char *aucBuf,int nBuf)
    {
    int n;

    n=BIO_read(pMachine->pbioWrite,aucBuf,nBuf);
    fprintf(stderr,"%d bytes of encrypted data read from state machine\n",n);
    return n;
    }

void SSLStateMachine_write_inject(SSLStateMachine *pMachine,
				  const unsigned char *aucBuf,int nBuf)
    {
    int n=SSL_write(pMachine->pSSL,aucBuf,nBuf);
    /* If it turns out this assert fails, then buffer the data here
     * and just feed it in in churn instead. Seems to me that it
     * should be guaranteed to succeed, though.
     */
    assert(n == nBuf);
    fprintf(stderr,"%d bytes of unencrypted data fed to state machine\n",n);
    }

void StateMachineEchoLoop(SSLStateMachine *pMachine, void* zmqSocket)
    {
    unsigned char rbuf[1];
    int nrbuf = 0;

    for( ; ; )
    {
    unsigned char buf[1024];
    int n;

    zmq_pollitem_t pitems[3] = {
        { 0, 0, ZMQ_POLLIN, 0 },          // stdin always selected for input
        { 0, 1, 0, 0 },                   // stdout
        { zmqSocket, -1, ZMQ_POLLIN, 0 }, // 0mq socket always select for input
    };

    unsigned int nitems = 3;
    unsigned int stdinIdx = 0;
    unsigned int stdoutIdx = 1;
    unsigned int socketIdx = 2;

    if ( ! SSL_is_init_finished(pMachine->pSSL) )
        SSL_do_handshake(pMachine->pSSL);

    /* TODO: SSL_get_verify_result() should be used to validate peer certs. */

    /* check whether there's decrypted data */
    if(!nrbuf)
        nrbuf=SSLStateMachine_read_extract(pMachine,rbuf,1);

    /* if there's decrypted data, check whether we can write it */
    if(nrbuf)
        pitems[stdoutIdx].events = ZMQ_POLLOUT;

    /* Select socket for output */
    if(SSLStateMachine_write_can_extract(pMachine))
        pitems[socketIdx].events |= ZMQ_POLLOUT;

    /* Wait for something to do something */
    n = zmq_poll(pitems, nitems, -1);
    assert(n > 0);

    /* Socket is ready for input */
    if ( pitems[socketIdx].revents & ZMQ_POLLIN )
        {
        zmq_msg_t msg;
        zmq_msg_init(&msg);
        zmq_recv(zmqSocket, &msg, 0);
        n = zmq_msg_size(&msg);
        unsigned char* msgbuf = malloc(n);
        memcpy(msgbuf, zmq_msg_data(&msg), n);
        zmq_msg_close(&msg);
        SSLStateMachine_read_inject(pMachine, msgbuf, n);
        free(msgbuf);
        }

    /* stdout is ready for output (and hence we have some to send it) */
    if ( pitems[stdoutIdx].revents & ZMQ_POLLOUT )
        {
        assert(nrbuf == 1);
        buf[0]=rbuf[0];
        nrbuf=0;

        n=SSLStateMachine_read_extract(pMachine,buf+1,sizeof buf-1);
        if(n < 0)
        {
        SSLStateMachine_print_error(pMachine,"read extract failed");
        break;
        }
        assert(n >= 0);
        ++n;
        if(n > 0) /* FIXME: has to be true now */
        {
        int w;

        w=write(1,buf,n);
        /* FIXME: we should push back any unwritten data */
        assert(w == n);
        }
        }

    /* Socket is ready for output (and therefore we have output to send) */
    if ( pitems[socketIdx].revents & ZMQ_POLLOUT )
        {
        n=SSLStateMachine_write_extract(pMachine,buf,sizeof buf);
        assert(n > 0);

        zmq_msg_t msg;
        zmq_msg_init_size(&msg, n);
        memcpy(zmq_msg_data(&msg), buf, n);
        zmq_send(zmqSocket, &msg, 0);
        zmq_msg_close(&msg);
        }

    /* Stdin is ready for input */
    if ( pitems[stdinIdx].revents & ZMQ_POLLIN )
        {
        n=read(0,buf,sizeof buf);
        if(n == 0)
        {
        fprintf(stderr,"Got EOF on stdin\n");
        exit(0);
        }
        assert(n > 0);

        SSLStateMachine_write_inject(pMachine,buf,n);
        }
    }
    /* not reached */
    }
