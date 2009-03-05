/*
** JNetLib
** Copyright (C) 2000-2006 CockOS, Inc.
** Author: Justin Frankel, Joshua Teitelbaum
** File: ssllisten.cpp - JNL SSL TCP listen implementation
** License: see jnetlib.h
*/

#include "netinc.h"
#include "util.h"
#include "ssllisten.h"

#ifdef _JNETLIB_SSL_


JNL_SSL_Listen::JNL_SSL_Listen(short port, unsigned long which_interface, const char *certpath, const char *privatekeypath) :
JNL_Listen(port,which_interface) ,m_app_ctx(NULL)
{
  SSL_METHOD *p_ssl_method;
  m_lasterror[0] = 0;
  unsigned long e;

  if(m_socket <= 0 || (certpath==NULL) || (privatekeypath==NULL))
  {
    return ;
  }

  SSL_load_error_strings();
  SSLeay_add_ssl_algorithms();

  p_ssl_method = SSLv23_server_method();
  m_app_ctx = SSL_CTX_new (p_ssl_method);

  if (!m_app_ctx)
  {
    e = ERR_get_error();
    ERR_error_string_n(e, m_lasterror, sizeof(m_lasterror) - 1);

	if (m_socket>=0)
    {
      closesocket(m_socket);
	  m_socket = -1;
    }
	return;
  }
   
  if (SSL_CTX_use_certificate_file(m_app_ctx, certpath, SSL_FILETYPE_PEM) <= 0)
  {
    e = ERR_get_error();
    ERR_error_string_n(e, m_lasterror, sizeof(m_lasterror) - 1);
	if (m_socket>=0)
    {
      closesocket(m_socket);
	  m_socket = -1;
    }
	return;
  }

  if (SSL_CTX_use_RSAPrivateKey_file(m_app_ctx, privatekeypath, SSL_FILETYPE_PEM) <= 0)
  {
    e = ERR_get_error();
    ERR_error_string_n(e, m_lasterror, sizeof(m_lasterror) - 1);
	if (m_socket>=0)
    {
      closesocket(m_socket);
	  m_socket = -1;
    }
	return;
  }

  if (!SSL_CTX_check_private_key(m_app_ctx))
  {
    e = ERR_get_error();
    ERR_error_string_n(e, m_lasterror, sizeof(m_lasterror) - 1);
	if (m_socket>=0)
    {
      closesocket(m_socket);
	  m_socket = -1;
    }
	return;
  }

  return;
}

JNL_SSL_Listen::~JNL_SSL_Listen()
{
  
}

JNL_Connection *JNL_SSL_Listen::get_connect(int sendbufsize, int recvbufsize)
{
  if(m_app_ctx == NULL)
  {
 	return NULL;
  }
 
  if (m_socket < 0)
  {
    return NULL;
  }
  struct sockaddr_in saddr;
  socklen_t length = sizeof(struct sockaddr_in);
  int s = accept(m_socket, (struct sockaddr *) &saddr, &length);

  if(s == -1)
  {
	  return NULL;
  }
  SSL* p_ssl = SSL_new(m_app_ctx);

  if( p_ssl == NULL || m_socket < 0)
  {
    sprintf(m_lasterror,"Could not create SSL Context");

	(m_socket>=0) ? closesocket(m_socket) : 0;
	m_socket = -1;

    if(p_ssl != NULL)
      SSL_free(p_ssl);

    return NULL;
  }

  else
  {
    JNL_Connection *c=new JNL_SSL_Connection(p_ssl,NULL,sendbufsize, recvbufsize);
    c->connect(s,&saddr);
    return c;
  }

  return NULL;
}

#endif
