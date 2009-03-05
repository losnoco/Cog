//
//  HTTPSource.h
//  HTTPSource
//
//  Created by Vincent Spader on 3/1/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <JNetLib/jnetlib.h>

#import "Plugin.h"

@interface HTTPSource : NSObject <CogSource>
{
	NSURLConnection *_connection;
	
	JNL_HTTPGet *_get;
	
	long _byteCount;
	
	NSString *_mimeType;
	
	NSURL *_url;
}

@end
