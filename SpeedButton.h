//
//  SpeedButton.h
//  Cog
//
//  Created by Christopher Snowhill on 9/20/24.
//  Copyright 2024 __LoSnoCo__. All rights reserved.
//

#import "SpeedSlider.h"
#import <Cocoa/Cocoa.h>

@interface SpeedButton : NSButton {
	IBOutlet SpeedSlider *_popView;
}

@end
