#import "TrackingSlider.h"
#import "TrackingCell.h"

@implementation TrackingSlider


static NSString *TrackingSliderValueObservationContext = @"TrackingSliderValueObservationContext";

- (id)initWithFrame:(NSRect)frameRect
{
	self = [super initWithFrame:frameRect];
	if (self)
	{
		bindingInfo = [[NSMutableDictionary alloc] init];
	}
	
	return self;
}

- (id)initWithCoder:(NSCoder *)decoder
{
	self = [super initWithCoder:decoder];
	if (self)
	{
		bindingInfo = [[NSMutableDictionary alloc] init];
	}
	
	return self;
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
	if ([TrackingSliderValueObservationContext isEqual:context])
	{
		if (![self isTracking])
		{
			id value = [change objectForKey:NSKeyValueChangeNewKey];
			[self setDoubleValue:[value doubleValue]];
		}
	}
	else
	{
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
	}
}

- (void)bind:(NSString *)binding toObject:(id)observableController withKeyPath:(NSString *)keyPath options:(NSDictionary *)options
{
	if ([binding isEqualToString:@"value"])
	{
		[observableController addObserver:self forKeyPath:keyPath options:(NSKeyValueObservingOptionNew) context:TrackingSliderValueObservationContext];
		
		NSDictionary *bindingsData = [NSDictionary dictionaryWithObjectsAndKeys:
									  observableController, NSObservedObjectKey,
									  [[keyPath copy] autorelease], NSObservedKeyPathKey,
									  [[options copy] autorelease], NSOptionsKey, nil];
		
		[bindingInfo setObject:bindingsData forKey:binding];
	}
	else
	{
		[super bind:binding toObject:observableController withKeyPath:keyPath options:options];
	}
}

- (void)unbind:(NSString *)binding
{
	if ([binding isEqualToString:@"value"])
	{
		NSDictionary *info = [bindingInfo objectForKey:binding];
		
		NSString *keyPath = [info objectForKey:NSObservedKeyPathKey];
		id observedObject = [info objectForKey:NSObservedObjectKey];
		
		[observedObject removeObserver:self forKeyPath:keyPath];
	}
	else
	{
		[super unbind:binding];
	}
}

- (BOOL)isTracking
{
	return [[self cell] isTracking];
}



@end
