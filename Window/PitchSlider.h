//
//  PitchSlider.h
//  Cog
//
//  Created by Christopher Snowhill on 9/20/24.
//  Copyright 2024 __LoSnoCo__. All rights reserved.
//

#import "TempoSlider.h"

#import <Cocoa/Cocoa.h>

@interface PitchSlider : NSSlider {
	NSPopover *popover;
	NSText *textView;
	IBOutlet NSSlider *_TempoSlider;
}

- (void)showToolTip;
- (void)showToolTipForDuration:(NSTimeInterval)duration;
- (void)showToolTipForView:(NSView *)view closeAfter:(NSTimeInterval)duration;
- (void)hideToolTip;

@end
