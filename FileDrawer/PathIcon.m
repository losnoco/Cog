//
//  FileIcon.m
//  Cog
//
//  Created by Vincent Spader on 8/20/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
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

- (void)dealloc
{
	[path release];
	[icon release];
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
