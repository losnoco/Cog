//
//  HTTPSource.m
//  HTTPSource
//
//  Created by Vincent Spader on 3/1/07.
//  Replaced by Christopher Snowhill on 3/7/20.
//  Copyright 2020 __LoSnoCo__. All rights reserved.
//

#import "HTTPSource.h"

#import "Logging.h"

#define BUFFER_SIZE 131072

@implementation HTTPSource

- (NSURLSession *)createSession
{
    queue = [[NSOperationQueue alloc] init];
    [queue setMaxConcurrentOperationCount:1];

    NSURLSession *session = nil;
    session = [NSURLSession sessionWithConfiguration:[NSURLSessionConfiguration defaultSessionConfiguration]
                                            delegate:self
                                       delegateQueue:queue];
    return session;
}

- (void)URLSession:(NSURLSession *)session
          dataTask:(NSURLSessionDataTask *)dataTask
    didReceiveData:(NSData *)data{
    long bytesBuffered = 0;
    if (cancelled) return;
    @synchronized(bufferedData) {
        [bufferedData addObject:data];
        _bytesBuffered += [data length];
        bytesBuffered = _bytesBuffered;
    }
    if (bytesBuffered > BUFFER_SIZE) {
        [task suspend];
    }
}

- (void)URLSession:(NSURLSession *)session dataTask:(NSURLSessionDataTask *)dataTask
 willCacheResponse:(NSCachedURLResponse *)proposedResponse
 completionHandler:(void (^)(NSCachedURLResponse *cachedResponse))completionHandler{
}

- (void)URLSession:(NSURLSession *)session
              task:(NSURLSessionTask *)task
didCompleteWithError:(NSError *)error{
    cancelled = YES;
    errorOccurred = YES;
}

- (BOOL)open:(NSURL *)url
{
    cancelled = NO;
    errorOccurred = NO;
    
    bufferedData = [[NSMutableArray alloc] init];
    
    URL = url;
    NSURLRequest * request = [NSURLRequest requestWithURL:url];
    session = [self createSession];
    task = [session dataTaskWithRequest:request];
    [task resume];
    
    NSURLResponse * response = nil;
    while (!response) {
        response = [task response];
        if (response) break;
        if (errorOccurred) return NO;
        usleep(100);
    }
    if (response) {
        _mimeType = [response MIMEType];
    }
    
	return YES;
}

- (NSString *)mimeType
{
	DLog(@"Returning mimetype! %@", _mimeType);
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

- (long)read:(void *)buffer amount:(long)amount
{
	long totalRead = 0;
    long bytesBuffered = 0;

	while (totalRead < amount) {
        NSData * dataBlock = nil;
        @synchronized(bufferedData) {
            if ([bufferedData count])
                dataBlock = [bufferedData objectAtIndex:0];
        }
        if (!dataBlock) {
            if (errorOccurred) return totalRead;
            if (cancelled) return 0;
            usleep(1000);
            continue;
        }
		NSInteger amountReceived = [dataBlock length];
		if (amountReceived <= 0) {
			break;
		}
        
        NSInteger amountUsed = amountReceived;
        
        if (amountUsed > (amount - totalRead) )
            amountUsed = amount - totalRead;
        
        const void * dataBytes = [dataBlock bytes];
        memcpy(((uint8_t *)buffer) + totalRead, dataBytes, amountUsed);
        
        if (amountUsed < amountReceived) {
            NSData * dataOut = [NSData dataWithBytes:(((uint8_t *)dataBytes) + amountUsed) length:(amountReceived - amountUsed)];
            @synchronized(bufferedData) {
                [bufferedData removeObjectAtIndex:0];
                [bufferedData insertObject:dataOut atIndex:0];
                _bytesBuffered -= amountUsed;
                bytesBuffered = _bytesBuffered;
            }
        }
        else {
            @synchronized(bufferedData) {
                [bufferedData removeObjectAtIndex:0];
                _bytesBuffered -= amountUsed;
                bytesBuffered = _bytesBuffered;
            }
        }
        
        if (bytesBuffered <= (BUFFER_SIZE * 3 / 4)) {
            [task resume];
        }

		totalRead += amountUsed;
	}
	
	_byteCount += totalRead;

	return totalRead;
}

- (void)close
{
    cancelled = YES;
    
    [task cancel];
    task = nil;
	
	_mimeType = nil;
}


- (void)dealloc
{
	[self close];
}

- (NSURL *)url
{
	return URL;
}

+ (NSArray *)schemes
{
	return [NSArray arrayWithObjects:@"http", @"https", nil];
}

@end
