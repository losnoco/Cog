//
//  SideWindowController.m
//  Cog
//
//  Created by Vincent Spader on 6/21/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "SideWindowController.h"


@implementation SideWindowController

- (id)init
{
	self = [super init];
	if (self)
	{
		[sideViewNibs setObject:@"FileTree" forKey:@"File Tree In a Window!"];
		[sideViewNibs setObject:@"FileTree" forKey:@"Another File Tree (In a window)!"];
		
		windows = [[NSMutableDictionary alloc] init];
	}
	
	return self;
}

- (void)dealloc
{
	[windows release];
	
	[super dealloc];
}

- (void)selectSideViewWithTitle:(NSString *)title
{
	NSPanel *window = [windows objectForKey:title];
	if (window == nil)
	{
		SideView *newSideView = [self sideViewForTitle:title];

		NSUInteger styleMask = (NSClosableWindowMask|NSMiniaturizableWindowMask|NSResizableWindowMask|NSTitledWindowMask|NSUtilityWindowMask);
		
		window = [[NSPanel alloc] initWithContentRect:[[newSideView view] frame] styleMask:styleMask backing:NSBackingStoreBuffered defer:NO];
		[window setContentView:[newSideView view]];
		
		[windows setObject:window forKey:title];
	}

	[window makeKeyAndOrderFront:self];
}


@end
