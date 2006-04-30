#import "TrackingSlider.h"
#import "TrackingCell.h"

@implementation TrackingSlider

- (id)initWithCoder:(NSCoder *)decoder
{
	self = [super initWithCoder:decoder];
	if (self)
	{
		if (![[self cell] isKindOfClass:[TrackingCell class]])
		{
			TrackingCell *trackingCell;
			trackingCell = [[TrackingCell alloc] init];
			
			[trackingCell setControlSize:[[self cell] controlSize]];
			[trackingCell setAction:[[self cell] action]];
			[trackingCell setContinuous:[[self cell] isContinuous]];
			[trackingCell setTarget:[[self cell] target]];
			[self setCell:trackingCell];
			
			[trackingCell release];
		}
	}
	
	return self;
}

+ (Class) cellClass
{
	return [TrackingCell class];
}

-(BOOL)tracking
{
	return [[self cell] tracking];
}

@end
