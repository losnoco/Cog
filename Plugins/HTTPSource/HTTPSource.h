//
//  HTTPSource.h
//  HTTPSource
//
//  Created by Vincent Spader on 3/1/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "Plugin.h"

@class HTTPConnection;

@interface HTTPSource : NSObject <CogSource, NSURLSessionDelegate, NSURLSessionTaskDelegate, NSURLSessionDataDelegate>
{
    NSOperationQueue * queue;
    
    NSURL * URL;
    NSURLSession * session;
    NSURLSessionDataTask * task;
    
    Boolean cancelled;
    Boolean errorOccurred;
    
    NSMutableArray * bufferedData;
	
    long _bytesBuffered;
	long _byteCount;
	
	NSString *_mimeType;
}



@end
