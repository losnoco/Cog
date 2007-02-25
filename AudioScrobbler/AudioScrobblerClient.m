/*
 *  $Id: AudioScrobblerClient.m 362 2007-02-13 05:30:49Z stephen_booth $
 *
 *  Copyright (C) 2006 - 2007 Stephen F. Booth <me@sbooth.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * This is a port of the BlockingClient client class from 
 * the Last.fm ScrobSub library by sharevari
 */

#import "AudioScrobblerClient.h"

#define kBufferSize		1024
#define	kPortsToStep	5

static in_addr_t 
addressForHost(NSString *hostname)
{
	NSCParameterAssert(nil != hostname);
	
	in_addr_t			address;
	struct hostent		*hostinfo;
	
	address = inet_addr([hostname cStringUsingEncoding:NSASCIIStringEncoding]);
	
    if(INADDR_NONE == address) {
        hostinfo = gethostbyname([hostname cStringUsingEncoding:NSASCIIStringEncoding]);
        if(NULL == hostinfo) {
			NSLog(@"Unable to resolve address for \"%@\".", hostname);
			return INADDR_NONE;
        }
		
        address = *((in_addr_t *)hostinfo->h_addr_list[0]);
    }

	return address;
}

@interface AudioScrobblerClient (Private)
- (void) connectToSocket:(in_addr_t)remoteAddress port:(in_port_t)port;
@end

@implementation AudioScrobblerClient

- (id) init
{
	if((self = [super init])) {
		_socket				= -1;
		_lastPort			= -1;
		_doPortStepping		= YES;
	}
	return self;
}

- (in_port_t) connectToHost:(NSString *)hostname port:(in_port_t)port
{
	in_addr_t	remoteAddress	= addressForHost(hostname);
	
	[self connectToSocket:remoteAddress port:port];
	
	return _lastPort;
}

- (void) send:(NSString *)data
{
	const char		*utf8data		= [data UTF8String];
	unsigned		len				= strlen(utf8data);	
	unsigned		bytesToSend		= len;
	unsigned		totalBytesSent	= 0;
	ssize_t			bytesSent		= 0;
	
	while(totalBytesSent < bytesToSend && -1 != bytesSent) {
		bytesSent = send(_socket, utf8data + totalBytesSent, bytesToSend - totalBytesSent, 0);
		
		if(-1 == bytesSent || 0 == bytesSent) {
			NSLog(@"Unable to send data through socket");
		}
		
		totalBytesSent += bytesSent;
	}
}

- (NSString *) receive
{
	char		buffer			[ kBufferSize ];
	int			readSize		= kBufferSize - 1;
	ssize_t		bytesRead		= 0;
	BOOL		keepGoing		= YES;
	NSString	*result			= nil;
	
	do {
		bytesRead = recv(_socket, buffer, readSize, 0);
		NSAssert1(-1 != bytesRead && 0 < bytesRead, @"Unable to receive data through socket (%s).", strerror(errno));
			
		if('\n' == buffer[bytesRead - 1]) {			
			--bytesRead;
			keepGoing = NO;
		}

		buffer[bytesRead]	= '\0';
		result				= [[NSString alloc] initWithUTF8String:buffer];
		
	} while(keepGoing);
	
	return [result autorelease];
}

- (void) shutdown
{
	int			result;
	char		buffer [ kBufferSize ];
	ssize_t		bytesRead;
	
	if(-1 == _socket) {
		return;
	}
	
	result = shutdown(_socket, SHUT_WR);
	NSAssert1(-1 != result, @"Socket shutdown failed (%s).", strerror(errno));

	for(;;) {
		bytesRead = recv(_socket, buffer, kBufferSize, 0);

		NSAssert1(-1 != bytesRead, @"Waiting for shutdown confirmation failed (%s).", strerror(errno));
		
		if(0 != bytesRead) {
			NSLog(@"Received unexpected bytes during shutdown: %@.", [[[NSString alloc] initWithBytes:buffer length:bytesRead encoding:NSUTF8StringEncoding] autorelease]);
		}
		else {
			break;
		}
	}

	result = close(_socket);
	NSAssert1(-1 != result, @"Couldn't close socket (%s).", strerror(errno));
	
	_socket = -1;
}

@end

@implementation AudioScrobblerClient (Private)

- (void) connectToSocket:(in_addr_t)remoteAddress port:(in_port_t)port
{
	struct sockaddr_in		socketAddress;
	int						result;

	_socket = socket(AF_INET, SOCK_STREAM, 0);
	NSAssert1(-1 != _socket, @"Unable to create socket (%s).", strerror(errno));

	_lastPort						= port;
	socketAddress.sin_family		= AF_INET;
	socketAddress.sin_addr.s_addr	= remoteAddress;
	socketAddress.sin_port			= htons(_lastPort);

	result = connect(_socket, (const struct sockaddr *)&socketAddress, sizeof(struct sockaddr_in));

	if(_doPortStepping) {
		while(-1 == result && _lastPort <= (port + kPortsToStep)) {
			socketAddress.sin_port = htons(++_lastPort);
			result = connect(_socket, (const struct sockaddr *)&socketAddress, sizeof(struct sockaddr_in));
		}
	}

	if(-1 == result) {
		_doPortStepping = NO;
		close(_socket);
		_socket = -1;
		NSAssert1(-1 != result, @"Couldn't connect to server (%s).", strerror(errno));
	}
}

@end
