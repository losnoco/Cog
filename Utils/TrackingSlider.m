#import "TrackingSlider.h"
#import "TrackingCell.h"

@implementation TrackingSlider

static NSString *TrackingSliderValueObservationContext = @"TrackingSliderValueObservationContext";

- (id)initWithFrame:(NSRect)frameRect {
	self = [super initWithFrame:frameRect];
	if(self) {
		bindingInfo = [NSMutableDictionary new];
	}

	return self;
}

- (id)initWithCoder:(NSCoder *)decoder {
	self = [super initWithCoder:decoder];
	if(self) {
		bindingInfo = [NSMutableDictionary new];
	}

	return self;
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
	if([TrackingSliderValueObservationContext isEqual:(__bridge id)(context)]) {
		if(![self isTracking]) {
			id value = [change objectForKey:NSKeyValueChangeNewKey];
			[self setDoubleValue:[value doubleValue]];
		}
	} else {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
	}
}

- (void)bind:(NSString *)binding toObject:(id)observableController withKeyPath:(NSString *)keyPath options:(NSDictionary *)options {
	if([binding isEqualToString:@"value"]) {
		[observableController addObserver:self forKeyPath:keyPath options:(NSKeyValueObservingOptionNew)context:(__bridge void *_Nullable)(TrackingSliderValueObservationContext)];

		NSDictionary *bindingsData = @{NSObservedObjectKey: observableController,
									   NSObservedKeyPathKey: [keyPath copy],
									   NSOptionsKey: options ? [options copy] : @{}};

		[bindingInfo setObject:bindingsData forKey:binding];
	} else {
		[super bind:binding toObject:observableController withKeyPath:keyPath options:options];
	}
}

- (void)unbind:(NSString *)binding {
	if([binding isEqualToString:@"value"]) {
		NSDictionary *info = [bindingInfo objectForKey:binding];

		NSString *keyPath = [info objectForKey:NSObservedKeyPathKey];
		id observedObject = [info objectForKey:NSObservedObjectKey];

		[observedObject removeObserver:self forKeyPath:keyPath];
	} else {
		[super unbind:binding];
	}
}

- (BOOL)isTracking {
	return [[self cell] isTracking];
}

- (void)mouseDown:(NSEvent *)event {
	TrackingCell *cell = [self cell];
	[cell setTracking:YES];
	BOOL handled = [self trackPointerFromEvent:event];
	[cell setTracking:NO];
	if(!handled) [super mouseDown:event];
}

@end

@implementation NSSlider (PointerTracking)

// The value whose knob centre is at the pointer: the knob's centre travels
// the bar less half the knob at either end.
- (double)valueAtPointerOfEvent:(NSEvent *)event {
	NSSliderCell *cell = [self cell];
	NSRect bar = [cell barRectFlipped:[self isFlipped]];
	CGFloat inset = [cell knobThickness] / 2;
	CGFloat span = NSWidth(bar) - 2 * inset;
	NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
	double fraction = span > 0 ? MIN(MAX((point.x - NSMinX(bar) - inset) / span, 0.0), 1.0) : 0.0;
	return [self minValue] + fraction * ([self maxValue] - [self minValue]);
}

// On macOS 15, NSSlider takes a click only on its knob or its thin bar,
// and a click on the knob drags it from where it was grabbed. So a click
// beside the knob or below the bar, as on the waveform, did nothing, and the
// knob did not follow the pointer exactly.
- (BOOL)trackPointerFromEvent:(NSEvent *)event {
	if(![self isEnabled] || [self isVertical]) return NO;

	[self setDoubleValue:[self valueAtPointerOfEvent:event]];
	if([self isContinuous]) [self sendAction:[self action] to:[self target]];

	while(YES) {
		NSEvent *next = [NSApp nextEventMatchingMask:NSEventMaskLeftMouseDragged | NSEventMaskLeftMouseUp
		                                   untilDate:[NSDate distantFuture]
		                                      inMode:NSEventTrackingRunLoopMode
		                                     dequeue:YES];
		if([next type] == NSEventTypeLeftMouseUp) break;
		double value = [self valueAtPointerOfEvent:next];
		if(value != [self doubleValue]) {
			[self setDoubleValue:value];
			if([self isContinuous]) [self sendAction:[self action] to:[self target]];
		}
	}

	if(![self isContinuous]) [self sendAction:[self action] to:[self target]];
	return YES;
}

@end
