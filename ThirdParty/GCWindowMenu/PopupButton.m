//
//  GCPopTestView.m
//  GCWindowMenuTest
//
//  Created by Graham on Tue Apr 24 2007.
//  Copyright (c) 2007 __MyCompanyName__. All rights reserved.
//

#import "PopupButton.h"
#import "GCWindowMenu.h"


@implementation PopupButton

- (void) mouseDown:(NSEvent*) event
{
	// create the menu to pop up using the view connected to the _popView outlet
	
	GCWindowMenu* menu = [GCWindowMenu windowMenuWithContentView:_popView];
	[menu setShouldCloseWhenViewTrackingReturns:YES];
	
	// place the menu relative to our window
	NSPoint p = [self bounds].origin;
	p.y += [self bounds].size.height;
	p.x += ([self bounds].size.width - [[[_popView window] contentView] bounds].size.width)/2.0;
	p = [self convertPoint:p toView:nil];
	
	// draw self highlighted
	[self highlight:YES];
	
	// pop up and track the menu
	[menu popUpAtPoint:p withEvent:event];

	// unhighlight self
	[self highlight:NO];
}

@end
