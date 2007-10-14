//
//  FileSource.h
//  FileSource
//
//  Created by Vincent Spader on 3/1/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "Plugin.h"

@interface FileSource : NSObject <CogSource>
{
	FILE *_fd;
	
	NSURL *_url;
}

@end
