//
//  VolumeSlider.h
//  Cog
//
//  Created by Vincent Spader on 2/8/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "ToolTipWindow.h"

@interface VolumeSlider : NSSlider {
	ToolTipWindow *toolTip;
}

- (void)showToolTip;
- (void)showToolTipForDuration:(NSTimeInterval)duration;
- (void)hideToolTip;

@end
