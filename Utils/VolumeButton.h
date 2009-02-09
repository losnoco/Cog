//
//  VolumeButton.h
//  Cog
//
//  Created by Vincent Spader on 2/8/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "PopupButton.h"

@class PlaybackController;

@interface VolumeButton : PopupButton {
	IBOutlet PlaybackController *playbackController;
}

@end
