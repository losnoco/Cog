/* TrackingSlider

 This is an ubersimple subclass of NSSlider that
 exposes a tracking method on the cell which can
 be used to tell if the user is currently dragging the slider.
 This is used in the action of the slider (the slider action is
 sent continuously) so the position text label is updated,
 without actually seeking the song until the mouse is released.

 */

#import <Cocoa/Cocoa.h>

@interface TrackingSlider : NSSlider {
	NSMutableDictionary *bindingInfo;
}

- (BOOL)isTracking;

@end

@interface NSSlider (PointerTracking)

/// Puts the knob under the pointer and keeps it there until the mouse goes
/// up, anywhere across the slider's height. Returns NO, doing nothing, for a
/// disabled or vertical slider.
- (BOOL)trackPointerFromEvent:(NSEvent *)event;

@end
