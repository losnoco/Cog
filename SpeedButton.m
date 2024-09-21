//
//  SpeedButton.m
//  Cog
//
//  Created by Christopher Snowhill on 9/20/24.
//  Copyright 2024 __LoSnoCo__. All rights reserved.
//

#import "SpeedButton.h"
#import "PlaybackController.h"

@implementation SpeedButton {
	NSPopover *popover;
	NSViewController *viewController;
}

- (void)awakeFromNib {
	popover = [[NSPopover alloc] init];
	popover.behavior = NSPopoverBehaviorTransient;
	[popover setContentSize:_popView.bounds.size];
}

- (void)scrollWheel:(NSEvent *)theEvent {
	if([popover isShown]) {
		[_popView scrollWheel:theEvent];
		return;
	}

	double change = [theEvent deltaY];

	[_popView setDoubleValue:[_popView doubleValue] + change];

	[[_popView target] changeSpeed:_popView];

	[_popView showToolTipForView:self closeAfter:1.0];
}

- (void)mouseDown:(NSEvent *)theEvent {
	[popover close];

	popover.contentViewController = nil;
	viewController = [[NSViewController alloc] init];
	viewController.view = _popView;
	popover.contentViewController = viewController;

	[popover showRelativeToRect:self.bounds ofView:self preferredEdge:NSRectEdgeMaxY];

	[super mouseDown:theEvent];
}

@end
