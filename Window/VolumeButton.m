//
//  VolumeButton.m
//  Cog
//
//  Created by Vincent Spader on 2/8/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "VolumeButton.h"
#import "PlaybackController.h"

@implementation VolumeButton {
	NSPopover *popover;
	NSViewController *viewController;
}

- (void)awakeFromNib {
	popover = [NSPopover new];
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

	[[_popView target] changeVolume:_popView];

	[_popView showToolTipForView:self closeAfter:1.0];
}

- (void)mouseDown:(NSEvent *)theEvent {
	[popover close];

	popover.contentViewController = nil;
	viewController = [NSViewController new];
	viewController.view = _popView;
	popover.contentViewController = viewController;

	[popover showRelativeToRect:self.bounds ofView:self preferredEdge:NSRectEdgeMaxY];

	[super mouseDown:theEvent];
}

@end
