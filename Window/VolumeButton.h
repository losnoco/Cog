//
//  VolumeButton.h
//  Cog
//
//  Created by Vincent Spader on 2/8/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "VolumeSlider.h"

@interface VolumeButton : NSButton {
    IBOutlet VolumeSlider *_popView;
}

@end
