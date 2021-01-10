//
//  VolumeSlider.h
//  Cog
//
//  Created by Vincent Spader on 2/8/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface VolumeSlider : NSSlider {
    NSPopover *popover;
    NSText *textView;
}

- (void)showToolTip;
- (void)showToolTipForDuration:(NSTimeInterval)duration;
- (void)showToolTipForView:(NSView *)view closeAfter:(NSTimeInterval)duration;
- (void)hideToolTip;

@end
