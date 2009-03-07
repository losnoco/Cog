//
//  Socket.m
//  Cog
//
//  Created by Vincent Spader on 2/28/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "Socket.h"
#import <netdb.h>

@implementation Socket

+ (id)socketWithHost:(NSString *)host port:(NSInteger)port
{
	return [[[Socket alloc] initWithHost:host port:port] autorelease];
}

- (id)initWithHost:(NSString *)host port:(NSInteger) port
{
	self = [super init];
	if (self)
	{
		_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		struct sockaddr_in sin;
		struct hostent *he;

		if (_socket < 0) {
		   return nil;
		}

		sin.sin_family = AF_INET;
		sin.sin_port = htons(port);

		he = gethostbyname([host UTF8String]);
		if (!he) {
		   NSLog(@"Socket error.");
		   close(_socket);
		   return nil;
		}
		memcpy(&sin.sin_addr, he->h_addr, 4);

		if (connect(_socket, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		   NSLog(@"Error: %s\n", strerror(errno));
		   close(_socket);
		   return nil;
		}
	}
	
	return self;
}


- (NSInteger)send:(const void *)data amount:(NSInteger)amount
{
	return send(_socket, data, amount, 0);
}

- (NSInteger)receive:(void *)data amount:(NSInteger)amount
{
	return recv(_socket, data, amount, 0);
}

- (void)close
{
	close(_socket);
}

- (void)dealloc
{
	[self close];
	
	[super dealloc];
}

@end
