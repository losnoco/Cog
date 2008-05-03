/* TrackingSlider 
 
 This is an ubersimple subclass of NSSlider that 
 exposes a tracking method on the cell which can
 be used to tell if the user is currently dragging the slider.
 This is used in the action of the slider (the slider action is 
 sent continuously) so the position text label is updated, 
 without actually seeking the song until the mouse is released.
 
 */

#import <Cocoa/Cocoa.h>

@interface TrackingSlider : NSSlider
{
}
-(BOOL)tracking;

@end
