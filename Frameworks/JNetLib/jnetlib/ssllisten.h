/*
** JNetLib
** Copyright (C) 2000-2001 Nullsoft, Inc.
** Author: Justin Frankel, Joshua Teitelbaum
** File: ssllisten.h - JNL interface for opening a SSL TCP listen
** License: see jnetlib.h
**
** Usage:
**   1. create a JNL_Listen object with the port and (optionally) the interface
**      to listen on.
**   2. call get_connect() to get any new connections (optionally specifying what
**      buffer sizes the connection should be created with)
**   3. check is_error() to see if an error has occured
**   4. call port() if you forget what port the listener is on.
**
*/

#include "netinc.h"
#include "listen.h"
#ifdef _JNETLIB_SSL_

#ifndef _SSL_LISTEN_H_
#define _SSL_LISTEN_H_
#include "sslconnection.h"

#include <openssl/rsa.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/conf.h>

class JNL_SSL_Listen : public JNL_Listen
{
protected:
	SSL_CTX *m_app_ctx;
	char m_lasterror[1024];

  public:
    JNL_SSL_Listen(short port, unsigned long which_interface=0,const char *certpath=NULL, const char *privatekeypath=NULL);
    virtual ~JNL_SSL_Listen();

    virtual JNL_Connection *get_connect(int sendbufsize=8192, int recvbufsize=8192);

};

#endif //_SSL_LISTEN_H_
#endif //_JNETLIB_SSL_

