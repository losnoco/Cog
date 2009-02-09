//
//  VolumeButton.m
//  Cog
//
//  Created by Vincent Spader on 2/8/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "VolumeButton.h"
#import "VolumeSlider.h"

@implementation VolumeButton

- (void)scrollWheel:(NSEvent *)theEvent
{
	double change = [theEvent deltaY];
	
	[(VolumeSlider *)_popView setDoubleValue:[(VolumeSlider *)_popView doubleValue] + change];
	
	[playbackController changeVolume:_popView];
	
	[(VolumeSlider *)_popView showToolTipForDuration:1.0];
}

- (void)mouseDown:(NSEvent *)theEvent
{
	[(VolumeSlider *)_popView hideToolTip];
	
	[super mouseDown:theEvent];

	[(VolumeSlider *)_popView hideToolTip];
}


@end
