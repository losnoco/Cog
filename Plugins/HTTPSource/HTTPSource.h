//
//  HTTPSource.h
//  HTTPSource
//
//  Created by Vincent Spader on 3/1/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "Socket.h"
#import "Plugin.h"

@interface HTTPSource : NSObject <CogSource>
{
	Socket *_socket;
	
	NSURL *_url;
	
	BOOL pastHeader;
	
	long byteCount;
}

- (void)setURL:(NSURL *)url;

@end
