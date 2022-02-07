/* TrackingCell */

#import <Cocoa/Cocoa.h>

@interface TrackingCell : NSSliderCell {
	BOOL tracking;
}

- (BOOL)startTrackingAt:(NSPoint)startPoint inView:(NSView *)controlView;
- (void)stopTracking:(NSPoint)lastPoint at:(NSPoint)stopPoint inView:(NSView *)controlView mouseIsUp:(BOOL)flags;

- (BOOL)isTracking;

@end
