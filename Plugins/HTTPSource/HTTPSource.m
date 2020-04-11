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

#import <stdlib.h>
#import <string.h>

#define BUFFER_SIZE 262144

@implementation HTTPSource

- (NSURLSession *)createSession
{
    queue = [[NSOperationQueue alloc] init];

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
    if (!task) return;
    if (didReceiveRandomData) {
        // Parse ICY header here?
        // XXX
        didReceiveRandomData = NO;
        
        const char * header = "ICY 200 OK\r\n";
        size_t length = [data length];
        if (length >= strlen(header)) {
            const char * dataBytes = (const char *) [data bytes];
            const char * dataStart = dataBytes;
            if (memcmp(dataBytes, header, strlen(header)) == 0) {
                const char * dataEnd = dataBytes + length;
                Boolean endFound = NO;
                while (dataBytes + 4 <= dataEnd) {
                    if (memcmp(dataBytes, "\r\n\r\n", 4) == 0) {
                        endFound = YES;
                        break;
                    }
                    dataBytes++;
                }
                if (!endFound) {
                    @synchronized(task) {
                        didComplete = YES;
                        [task cancel];
                        task = nil;
                        return;
                    }
                }
                dataEnd = dataBytes + 4;
                NSUInteger dataLeft = length - (dataEnd - dataStart);
                dataBytes = dataStart;
                dataBytes += strlen("ICY 200 OK\r\n");
                char headerBuffer[80 * 1024 + 1];
                while (dataBytes < dataEnd - 2) {
                    const char * string = dataBytes;
                    while (dataBytes < dataEnd - 2) {
                        if (memcmp(dataBytes, "\r\n", 2) == 0) break;
                        dataBytes++;
                    }
                    if (dataBytes - string > 80 * 1024)
                        dataBytes = string + 80 * 1024;
                    strncpy(headerBuffer, string, dataBytes - string);
                    headerBuffer[dataBytes - string] = '\0';
                    
                    char * colon = strchr(headerBuffer, ':');
                    if (colon) {
                        *colon = '\0';
                        colon++;
                    }
                    
                    if (strcasecmp(headerBuffer, "content-type") == 0) {
                        _mimeType = [NSString stringWithUTF8String:colon];
                    }
                    
                    dataBytes += 2;
                }
                
                data = [NSData dataWithBytes:dataEnd length:dataLeft];
                
                didReceiveResponse = YES;
            }
        }
    }
    @synchronized(bufferedData) {
        [bufferedData appendData:data];
        _bytesBuffered += [data length];
        bytesBuffered = _bytesBuffered;
    }
    if (bytesBuffered >= BUFFER_SIZE) {
        [task suspend];
    }
}

- (void)URLSession:(NSURLSession *)session
          dataTask:(NSURLSessionDataTask *)dataTask
didReceiveResponse:(NSURLResponse *)response
 completionHandler:(void (^)(NSURLSessionResponseDisposition disposition))completionHandler {
    if ([response isKindOfClass:[NSHTTPURLResponse class]]) {
        NSInteger statusCode = [(NSHTTPURLResponse *)response statusCode];
        if (statusCode != 200) {
            completionHandler(NSURLSessionResponseCancel);
            @synchronized (task) {
                task = nil;
            }
            return;
        }
    }
    _mimeType = [response MIMEType];
    if ([_mimeType isEqualToString:@"application/octet-stream"] ||
        [_mimeType isEqualToString:@"text/plain"])
        didReceiveRandomData = YES;
    else
        didReceiveResponse = YES;
    
    completionHandler(NSURLSessionResponseAllow);
}

- (void)URLSession:(NSURLSession *)session
              task:(NSURLSessionTask *)task
willPerformHTTPRedirection:(NSHTTPURLResponse *)response
        newRequest:(NSURLRequest *)request
 completionHandler:(void (^)(NSURLRequest *))completionHandler {
    NSURL * url = [request URL];
    if ([redirectURLs containsObject:url]) {
        completionHandler(NULL);
        @synchronized(self->task) {
            self->task = nil;
        }
    }
    else {
        [redirectURLs addObject:url];
        redirected = YES;
        didReceiveResponse = NO;
        didComplete = NO;
        @synchronized(bufferedData) {
            [bufferedData setLength:0];
            _bytesBuffered = 0;
        }
        completionHandler(request);
    }
}

- (void)URLSession:(NSURLSession *)session
didBecomeInvalidWithError:(NSError *)error {
    @synchronized(task) {
        task = nil;
    }
}

- (void)URLSession:(NSURLSession *)session dataTask:(NSURLSessionDataTask *)dataTask
 willCacheResponse:(NSCachedURLResponse *)proposedResponse
 completionHandler:(void (^)(NSCachedURLResponse *cachedResponse))completionHandler{
    didComplete = YES;
}

- (void)URLSession:(NSURLSession *)session
              task:(NSURLSessionTask *)task
didCompleteWithError:(NSError *)error{
    @synchronized(self->task) {
        self->task = nil;
    }
}

- (BOOL)open:(NSURL *)url
{
    didReceiveResponse = NO;
    didReceiveRandomData = NO;
    redirected = NO;
    
    redirectURLs = [[NSMutableArray alloc] init];
    bufferedData = [[NSMutableData alloc] init];
    
    URL = url;
    [redirectURLs addObject:URL];
    
    NSURLRequest * request = [NSURLRequest requestWithURL:url];
    session = [self createSession];
    task = [session dataTaskWithRequest:request];
    [task resume];
    
    while (task && !didReceiveResponse)
        usleep(1000);

    if (!task) return NO;
    
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
    if (didComplete)
        return 0;
    
	long totalRead = 0;
    long bytesBuffered = 0;

	while (totalRead < amount) {
        NSData * dataBlock = nil;
        NSUInteger copySize = amount - totalRead;
        @synchronized(bufferedData) {
            if ([bufferedData length]) {
                if (copySize > [bufferedData length])
                    copySize = [bufferedData length];
                dataBlock = [bufferedData subdataWithRange:NSMakeRange(0, copySize)];
            }
        }
        if (!dataBlock) {
            @synchronized(task) {
                if (!task || didComplete) return totalRead;
            }
            usleep(1000);
            continue;
        }
		NSInteger amountReceived = [dataBlock length];
		if (amountReceived <= 0) {
			break;
		}
        
        const void * dataBytes = [dataBlock bytes];
        memcpy(((uint8_t *)buffer) + totalRead, dataBytes, amountReceived);
        
        @synchronized(bufferedData) {
            [bufferedData replaceBytesInRange:NSMakeRange(0, amountReceived) withBytes:NULL length:0];
            _bytesBuffered -= amountReceived;
            bytesBuffered = _bytesBuffered;
        }
        
        if (!didComplete && bytesBuffered <= (BUFFER_SIZE * 3 / 4)) {
            [task resume];
        }

		totalRead += amountReceived;
	}
	
	_byteCount += totalRead;

	return totalRead;
}

- (void)close
{
    if (task) [task cancel];
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
