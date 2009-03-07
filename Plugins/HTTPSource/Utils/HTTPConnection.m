//
//  HTTPConnection.m
//  HTTPSource
//
//  Created by Vincent Spader on 3/6/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "HTTPConnection.h"

#include <netdb.h>


@implementation HTTPConnection

@synthesize URL = _URL;

- (id)initWithURL:(NSURL *)url
{
	self = [super init];
	if (self)
	{
		[self setURL:url];
		
		_requestHeaders = nil;
		_responseHeaders = nil;
		_buffer = NULL;
		_bufferSize = 0;
	}
	
	return self;
}

- (void)dealloc
{
	[_requestHeaders release];
	[_responseHeaders release];
	
	[self setURL:nil];
	
	[super dealloc];
}

- (void)setValue:(NSString *)value forRequestHeader:(NSString *)header
{
	if (nil == _requestHeaders) {
		_requestHeaders = [[NSMutableDictionary alloc] init];
	}
	
	[_requestHeaders setObject:value forKey:header];
}

// Only filled in after a successful -connect
- (NSString *)valueForResponseHeader:(NSString *)header
{
	return [_responseHeaders objectForKey:[header lowercaseString]];
}

- (NSString *)_receiveLine
{
	const NSInteger requestAmount = 1024;
	
	void *newLine = strnstr((const char *)_buffer, "\r\n", _bufferSize);
	while (NULL == newLine) {
		NSInteger bufferAmount = _bufferSize;
		
		// Note that _bufferSize may be 0 and _buffer may be NULL
		_bufferSize += requestAmount;
		_buffer = realloc(_buffer, _bufferSize);
		if (NULL == _buffer) {
			return nil;
		}
		
		NSInteger amountReceived = [_socket receive:((uint8_t *)_buffer)+bufferAmount amount:requestAmount];
		if (amountReceived <= 0) {
			// -close will free _buffer
			return nil;
		}
		
		if (amountReceived < requestAmount) {
			_bufferSize = bufferAmount + amountReceived;
		}
		
		newLine = strnstr((const char *)_buffer, "\r\n", _bufferSize);
	}
	
	NSInteger lineLength = ((uint8_t *)newLine - (uint8_t *)_buffer);
	NSString *line = [[NSString alloc] initWithBytes:_buffer length:lineLength encoding:NSASCIIStringEncoding];
	NSLog(@"Received line: \"%@\"", line);
	memmove(_buffer, _buffer + lineLength + 2, _bufferSize - lineLength); // + 2 to skip the newline!
	_bufferSize -= lineLength;
	
	return [line autorelease];
}

- (BOOL)_readResponse
{
	// Clear out any old response headers
	[_responseHeaders release];
	_responseHeaders = nil;
	
	// Fetch the first line so we can parse the status code
	NSString *firstLine = [self _receiveLine];
	if (nil == firstLine) {
		return NO;
	}
	
	// Get the status code!
	NSInteger statusCode = 0;
	NSScanner *scanner = [[NSScanner alloc] initWithString:firstLine];
	
	// Scan up to the space and the number afterwards
	BOOL success = ([scanner scanUpToString:@" " intoString:nil] && [scanner scanInteger:&statusCode]);
	[scanner release];
	if (NO == success) {
		// Failed to retrieve status code.
		return NO;
	}
	
	// Prepare for response!
	_responseHeaders = [[NSMutableDictionary alloc] init];
	
	// Go through the response headers
	BOOL foundEndOfHeaders = NO;
	while (NO == foundEndOfHeaders)
	{
		NSString *line = [self _receiveLine];
		if (nil == line) {
			// Error receiving data. Let's get out of here!
			NSLog(@"Headers ended prematurely");
			break;
		}
		
		if ([@"" isEqualToString:line]) {
			// We have \n\n, end of headers!
			foundEndOfHeaders = YES;
			continue;
		}
		
		// Add the header to the dict
		NSString *key = nil;
		NSString *value = nil;

		NSScanner *scanner = [[NSScanner alloc] initWithString:line];
		BOOL success = ([scanner scanUpToString:@":" intoString:&key] && [scanner scanString:@":" intoString:nil] && [scanner scanUpToString:@"" intoString:&value]);
		[scanner release];
		
		if (NO == success) {
			NSLog(@"Could not scan header: %@", line);
			continue;
		}
		
		[_responseHeaders setObject:value forKey:[key lowercaseString]];
	}
	
	if (200 == statusCode || 206 == statusCode) { // OK
		return YES;
	}
	else if (301 == statusCode || 302 == statusCode) { // Redirect
		NSURL *redirectURL = [[NSURL alloc] initWithString:[self valueForResponseHeader:@"Location"]];
		[self setURL:redirectURL];
		[self close];
		return [self connect];
	}
	
	NSLog(@"Returned status: %i", statusCode);
	return NO;
}

- (BOOL)_sendRequest
{
	NSURL *url = [self URL];
	
	// The initial GET
	NSMutableString *requestString = [[NSMutableString alloc] initWithFormat:@"GET %@ HTTP/1.0\r\n", [url path]];
	
	// Make sure there is a Host entry
	NSString *host = [url host];
	if (nil != [url port]) {
		host = [NSString stringWithFormat:@"%@:%@", [url host], [url port]];
	}
	
	[self setValue:host forRequestHeader:@"Host"];
	
	// Add the request headers
	for (id header in _requestHeaders) {
		id value = [_requestHeaders objectForKey:header];
		
		[requestString appendFormat:@"%@: %@\r\n", header, value];
	}

	// The final newline
	[requestString appendString:@"\r\n"];
	
	// Get the bytes out of it
	const char *requestBytes = [requestString UTF8String];
	int requestLength = strlen(requestBytes);
	
	// Send it off!
	NSInteger sent = [_socket send:requestBytes amount:requestLength];
	if (sent != requestLength) {
		return NO;
	}
	NSLog(@"Sent:\n%@\n", requestString);
	[requestString release];
	
	return YES;
}


// Returns YES for success, NO for...not success
- (BOOL)connect
{
	NSURL *url = [self URL];
	NSString *host = [url host];

	// Get the port number
	NSNumber *portNumber = [url port];
	NSInteger port = [portNumber integerValue];
	if (portNumber == nil) {
		port = 80; // Default for HTTP
	}
	
	// Cogundo...Makin' sockets
	_socket = [[Socket alloc] initWithHost:host port:port];
	if (nil == _socket) {
		return NO;
	}
	
	if (![self _sendRequest]) {
		return NO;
	}
	
	if (![self _readResponse]) {
		return NO;
	}
	
	return YES;
}

- (void)close
{
	[_socket close];
	[_socket release];
	_socket = nil;
	
	if (NULL != _buffer) {
		free(_buffer);
		_buffer = NULL;
		_bufferSize = 0;
	}
}

// Returns negative on timeout or error

- (NSInteger)receiveData:(void *)bytes amount:(NSInteger)amount
{
	NSInteger amountToRequest = amount;
	NSInteger amountToCopy = MIN(amount, _bufferSize);
	if (NULL != _buffer) {
		memcpy(bytes, _buffer, amountToCopy);
		_bufferSize -= amountToCopy;
		
		amountToRequest -= amountToCopy;
	}

	
	NSInteger bytesReceived = amountToCopy + [_socket receive:((uint8_t *)bytes) + amountToCopy amount:amountToRequest];

	return bytesReceived;
}

@end
