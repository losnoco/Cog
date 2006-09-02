//
//  FileIcon.m
//  Cog
//
//  Created by Zaphod Beeblebrox on 8/20/06.
//  Copyright 2006 __MyCompanyName__. All rights reserved.
//

#import "PathIcon.h"


@implementation PathIcon

-(id)initWithPath:(NSString *)p
{
	self = [super init];
	if (self)
	{
		path = [p retain];
		icon = [[[NSWorkspace sharedWorkspace] iconForFile:path] retain];
		
		[icon setSize: NSMakeSize(16.0, 16.0)];
	}
	
	return self;
}

- (NSString *) path 
{
	return path;
}

- (NSImage *) icon
{
	return icon;
}

@end
