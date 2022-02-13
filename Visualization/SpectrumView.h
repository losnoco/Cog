//
//  SpectrumView.h
//  Cog
//
//  Created by Christopher Snowhill on 2/12/22.
//

#import <Cocoa/Cocoa.h>

#import "VisualizationController.h"

NS_ASSUME_NONNULL_BEGIN

@interface SpectrumView : NSImageView {
	VisualizationController *visController;
	NSTimer *timer;
	NSImage *theImage;
	BOOL stopped;
}
@end

NS_ASSUME_NONNULL_END
