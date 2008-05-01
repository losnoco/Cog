#import "TrackingSlider.h"
#import "TrackingCell.h"

@implementation TrackingSlider

-(BOOL)tracking
{
	return [[self cell] tracking];
}

@end
