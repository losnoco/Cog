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
}

- (void)awakeFromNib
{
    popover = [[NSPopover alloc] init];

    NSViewController * viewController = [[NSViewController alloc] init];
    viewController.view = _popView;
    popover.contentViewController = viewController;
    popover.behavior = NSPopoverBehaviorTransient;
    [popover setContentSize:_popView.bounds.size];
}

- (void)scrollWheel:(NSEvent *)theEvent
{
    if ([popover isShown]) {
        [_popView scrollWheel:theEvent];
        return;
    }

    double change = [theEvent deltaY];

    [_popView setDoubleValue:[_popView doubleValue] + change];

    [[_popView target] changeVolume:_popView];

    [_popView showToolTipForView:self closeAfter:1.0];
}

- (void)mouseDown:(NSEvent *)theEvent
{
    [_popView hideToolTip];
    [popover showRelativeToRect:self.bounds ofView:self preferredEdge:NSRectEdgeMaxY];

    [super mouseDown:theEvent];
}


@end
