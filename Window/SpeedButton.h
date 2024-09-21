//
//  VolumeButton.h
//  Cog
//
//  Created by Vincent Spader on 2/8/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "VolumeSlider.h"
#import <Cocoa/Cocoa.h>

@interface VolumeButton : NSButton {
	IBOutlet VolumeSlider *_popView;
}

@end
