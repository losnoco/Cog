//
//  SpectrumView.h
//  Cog
//
//  Created by Christopher Snowhill on 2/12/22.
//

#import <Cocoa/Cocoa.h>

#import "VisualizationController.h"

NS_ASSUME_NONNULL_BEGIN

@interface SpectrumView : NSView {
	VisualizationController *visController;
	NSTimer *timer;
	BOOL paused;
	BOOL stopped;
	BOOL isListening;

	float FFTMax[256];

	NSColor *baseColor;
	NSColor *peakColor;
	NSColor *backgroundColor;
}
@property(nonatomic) BOOL isListening;
@end

NS_ASSUME_NONNULL_END
