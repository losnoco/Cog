#import "TrackingCell.h"

@implementation TrackingCell

- (BOOL)startTrackingAt:(NSPoint)startPoint inView:(NSView *)controlView
{
	tracking = YES;
	return [super startTrackingAt:startPoint inView:controlView];
	
}


- (BOOL)continueTracking:(NSPoint)lastPoint at:(NSPoint)currentPoint inView:(NSView *)controlView
{
	NSEvent *event = [NSEvent mouseEventWithType:NSEventTypeLeftMouseDragged location:currentPoint modifierFlags:0 timestamp:0 windowNumber:[[controlView window] windowNumber] context:nil eventNumber:0 clickCount:0 pressure:0];
	
	[controlView mouseDragged:event];
	
	return [super continueTracking:lastPoint at:currentPoint inView:controlView];
}

- (void)stopTracking:(NSPoint)lastPoint at:(NSPoint)stopPoint inView:(NSView *)controlView mouseIsUp:(BOOL)flag
{
	tracking = NO;

	[super stopTracking:lastPoint at:stopPoint inView:controlView mouseIsUp:flag];
}

- (BOOL)isTracking
{
	return tracking;
}

@end
