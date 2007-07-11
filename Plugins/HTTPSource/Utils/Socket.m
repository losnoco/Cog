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

+ (id)socketWithHost:(NSString *)host port:(unsigned int)port
{
	return [[[Socket alloc] initWithHost:host port:port] autorelease];
}

- (id)initWithHost:(NSString *)host port:(unsigned int) port
{
	self = [super init];
	if (self)
	{
		_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		struct sockaddr_in sin;
		struct hostent *he;

		if (_fd < 0) {
		   return nil;
		}

		sin.sin_family = AF_INET;
		sin.sin_port = htons(port);

		he = gethostbyname([host UTF8String]);
		if (!he) {
		   NSLog(@"Socket error.");
		   close(_fd);
		   return nil;
		}
		memcpy(&sin.sin_addr, he->h_addr, 4);

		if (connect(_fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		   NSLog(@"Error: %s\n", strerror(errno));
		   close(_fd);
		   return nil;
		}
	}
	
	return self;
}


- (int)send:(const void *)data amount:(unsigned int)amount
{
	return send(_fd, data, amount, 0);
}

- (int)receive:(void *)data amount:(unsigned int)amount
{
	return recv(_fd, data, amount, 0);
}

- (void)close
{
	close(_fd);
}

- (void)dealloc
{
	[self close];
	
	[super dealloc];
}

@end
