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
	
	_connectionFinished = NO;
	_byteCount = 0;
	_data = [[NSMutableData alloc] init];
	_sem = [[Semaphore alloc] init];
	
	[NSThread detachNewThreadSelector:@selector(makeConnection) toTarget:self withObject:nil];

	NSLog(@"Connection opened!");

	return YES;
}

- (void)makeConnection
{
	NSURLRequest *request = [[NSURLRequest alloc] initWithURL:_url];
	
	_connection = [[NSURLConnection alloc] initWithRequest:request delegate:self];
	
	[request release];
	
	[[NSRunLoop currentRunLoop] run];
	
}

- (NSDictionary *)properties
{
	return nil;
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
		NSLog(@"Waiting: %@", [NSThread currentThread]);
		[_sem timedWait: 2];
	}
	
	NSLog(@"Read called!");
	
	if (amount > [_data length])
		amount = [_data length];
	
	[_data getBytes:buffer length:amount];

	//Remove the bytes
	[_data replaceBytesInRange:NSMakeRange(0, amount) withBytes:NULL length:0];
	
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
	
	[NSThread exit];
}

- (void)close
{
	[_connection cancel];
	[_connection release];
	_connection = nil;
	
	[_data release];
	_data = nil;

	[_url release];
	_url = nil;
 
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
	//May be called more than once. Mime-type may change. Will be called before didReceiveData
	NSLog(@"Received response: %@", response);
	
	[_data release];
	_data = [[NSMutableData alloc] init];
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

	[NSThread exit];
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data
{
	NSLog(@"Connection received data.");

	[_data appendData:data];
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
