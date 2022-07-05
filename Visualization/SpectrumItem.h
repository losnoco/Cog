//
//  SpectrumItem.h
//  Cog
//
//  Created by Christopher Snowhill on 2/13/22.
//

#import <Cocoa/Cocoa.h>

#import "PlaybackController.h"

NS_ASSUME_NONNULL_BEGIN

@interface SpectrumItem : NSToolbarItem {
	IBOutlet PlaybackController *playbackController;
}
@end

NS_ASSUME_NONNULL_END
