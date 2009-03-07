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

@interface HTTPSource : NSObject <CogSource>
{
	HTTPConnection *_connection;
	
	long _byteCount;
	
	NSString *_mimeType;
}



@end
