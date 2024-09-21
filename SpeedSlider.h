//
//  SpeedSlider.h
//  Cog
//
//  Created by Christopher Snowhill on 9/20/24.
//  Copyright 2024 __LoSnoCo__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface SpeedSlider : NSSlider {
	NSPopover *popover;
	NSText *textView;
}

- (void)showToolTip;
- (void)showToolTipForDuration:(NSTimeInterval)duration;
- (void)showToolTipForView:(NSView *)view closeAfter:(NSTimeInterval)duration;
- (void)hideToolTip;

@end
