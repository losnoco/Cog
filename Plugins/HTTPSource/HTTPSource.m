//
//  HTTPSource.m
//  HTTPSource
//
//  Created by Vincent Spader on 3/1/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "HTTPSource.h"


@implementation HTTPSource

- (BOOL)open:(NSURL *)url
{
	_url = [url copy];
	
	_responseReceived = NO;
	_connectionFinished = NO;
	_byteCount = 0;
	_data = [[NSMutableData alloc] init];
	_sem = [[Semaphore alloc] init];
	
	[NSThread detachNewThreadSelector:@selector(doConnection) toTarget:self withObject:nil];
	
	//Wait for a response.
	while (!_responseReceived && !_connectionFinished) {
		[_sem wait];
	}

	NSLog(@"Connection opened!");

	return YES;
}

- (void)doConnection
{
	NSURLRequest *request = [[NSURLRequest alloc] initWithURL:_url];
	
	_connection = [[NSURLConnection alloc] initWithRequest:request delegate:self];
	
	[request release];

	while (!_connectionFinished)
	{
		NSDate *date = [[NSDate alloc] init];

		[[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:date];

		[date release];
	}
	
	NSLog(@"Thread exit");
}

- (NSString *)mimeType
{
	NSLog(@"Returning mimetype! %@", _mimeType);
	return _mimeType;
}

- (BOOL)seekable
{
	return NO;
}

- (BOOL)seek:(long)position whence:(int)whence
{
	return NO;
}

- (long)tell
{
	return _byteCount;
}

- (int)read:(void *)buffer amount:(int)amount
{
	while (amount > [_data length] && !_connectionFinished) {
		[_sem timedWait: 2];
	}
	
	NSLog(@"Read called!");
	
	if (amount > [_data length])
		amount = [_data length];
	
	@synchronized (_data) {
		[_data getBytes:buffer length:amount];

		//Remove the bytes
		[_data replaceBytesInRange:NSMakeRange(0, amount) withBytes:NULL length:0];
	}
	
	_byteCount += amount;

	return amount;
}

//Only called from thread.
- (void)cancel
{
	NSLog(@"CANCEL!");
	
	[_connection cancel];
	_connectionFinished = YES;

	[_sem signal];
}

- (void)close
{
	NSLog(@"CLOSING HTTPSource!");
	_connectionFinished = YES;

	[_connection cancel];
	[_connection release];
	_connection = nil;
	
	[_data release];
	_data = nil;

	[_url release];
	_url = nil;
	
	[_mimeType release];
	_mimeType = nil;
 
	[_sem release];
	_sem = nil;
}

- (void)connection:(NSURLConnection *)connection didCancelAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge
{
	NSLog(@"Authentication cancelled");
	[self cancel];
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error
{
	NSLog(@"Connection failed: %@", error);
	[self cancel];
}

- (void)connection:(NSURLConnection *)connection didReceiveAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge
{
	NSLog(@"Received authentication challenge. Canceling.");
	[self cancel];
}

-(void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response
{
	_mimeType = [[response MIMEType] copy];
	_responseReceived = YES;

	NSLog(@"Received response: %@", _mimeType);
	
	[_sem signal];
}

-(NSCachedURLResponse *)connection:(NSURLConnection *)connection willCacheResponse:(NSCachedURLResponse *)cachedResponse
{
	NSLog(@"Received cache request");
	
	//No caching an HTTP stream
	return nil;
}

-(NSURLRequest *)connection:(NSURLConnection *)connection willSendRequest:(NSURLRequest *)request redirectResponse:(NSURLResponse *)redirectResponse
{
	NSLog(@"Received redirect");

	//Redirect away
	return request;
}

-(void)connectionDidFinishLoading:(NSURLConnection *)connection
{
	NSLog(@"Connection finished loading.");

	_connectionFinished = YES;
	
	[_sem signal];
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data
{
	@synchronized (_data) {
		[_data appendData:data];
	}
	
	[_sem signal];
}

- (void)dealloc
{
	[self close];
	
	[super dealloc];
}

- (NSURL *)url
{
	return _url;
}

+ (NSArray *)schemes
{
	return [NSArray arrayWithObject:@"http"];
}

@end
