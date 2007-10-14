//
//  HTTPSource.h
//  HTTPSource
//
//  Created by Vincent Spader on 3/1/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "Semaphore.h"
#import "Plugin.h"

@interface HTTPSource : NSObject <CogSource>
{
	NSURLConnection *_connection;
	
	long _byteCount;
	
	BOOL _responseReceived;
	BOOL _connectionFinished;

	NSMutableData *_data;
	Semaphore *_sem;
	
	NSString *_mimeType;
	
	NSURL *_url;
}

@end
