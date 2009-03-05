/*
** JNetLib
** Copyright (C) 2000-2006 CockOS, Inc.
** Author: Justin Frankel, Joshua Teitelbaum
** File: udpconnection.h - JNL SSL TCP connection interface
** License: see jnetlib.h
*/

#ifndef _UDPCONNECTION_H_
#define _UDPCONNECTION_H_

#include "connection.h"

class JNL_UDP_Connection : public JNL_Connection
{
public:
	JNL_UDP_Connection(struct sockaddr_in *listenon, JNL_AsyncDNS *dns=JNL_CONNECTION_AUTODNS, int sendbufsize=8192, int recvbufsize=8192);
	virtual ~JNL_UDP_Connection();
	virtual void connect(int sock, struct sockaddr_in *loc=NULL); // used by the listen object, usually not needed by users.

	/*
	**  Joshua Teitelbaum 1/27/2006 Adding new BSD socket analogues for SSL compatibility
	*/
protected:
	virtual void socket_shutdown();
	virtual int socket_recv(char *buf, int len, int options);
    virtual int socket_send(char *buf, int len, int options);
	virtual int socket_connect();


};
#endif