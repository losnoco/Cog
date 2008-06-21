//
//  HelperController.m
//  Cog
//
//  Created by Vincent Spader on 6/20/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "SideView.h"


@implementation SideView

- (id)initWithNibNamed:(NSString *)nibName controller:(SideViewController *)c
{
	self = [super init];
	if (self)
	{
		controller = c;
		
		BOOL r = [NSBundle loadNibNamed:nibName owner:self];
		NSLog(@"LOADED NIB: %i", r);
	}
	
	return self;
}

- (NSView *)view
{
	return view;
}

- (void) addToPlaylist:(NSArray *)urls
{
	[controller addToPlaylist:urls];
}
@end
