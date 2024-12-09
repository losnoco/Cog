//
//  SpeedButton.h
//  Cog
//
//  Created by Christopher Snowhill on 9/20/24.
//  Copyright 2024 __LoSnoCo__. All rights reserved.
//

#import "PitchSlider.h"
#import "TempoSlider.h"
#import <Cocoa/Cocoa.h>

@interface SpeedButton : NSButton {
	IBOutlet NSView *_popView;
	IBOutlet PitchSlider *_PitchSlider;
	IBOutlet TempoSlider *_TempoSlider;
	IBOutlet NSButton *_LockButton;
	IBOutlet NSButton *_ResetButton;
}

- (IBAction)pressLock:(id)sender;
- (IBAction)pressReset:(id)sender;

@end
