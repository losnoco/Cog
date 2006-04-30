//
//  ScrollableTextField.m
//  Cog
//
//  Created by Zaphod Beeblebrox on 4/30/06.
//  Copyright 2006 __MyCompanyName__. All rights reserved.
//

#import "DragScrollView.h"


@implementation DragScrollView

- (BOOL)acceptsFirstMouse:(NSEvent *)event {
	return YES;
}

- (void)mouseDown:(NSEvent *)event
{
	if([event type] == NSLeftMouseDown)
	{
		if ([event clickCount] == 2)
		{
			[[self contentView] scrollToPoint:NSMakePoint(0,0)];
		}
		
		downPoint = [event locationInWindow];
		downPoint = [[self contentView] convertPoint:downPoint fromView:nil];
	}
	
}

- (void)mouseDragged:(NSEvent *)event
{
	NSPoint p = [event locationInWindow];
	
	p = [self convertPoint:p fromView:nil];

	p.y = 0;
	p.x = -p.x + downPoint.x;
	[[self contentView] scrollToPoint:p];
	[self setNeedsDisplay:YES];
}


@end
