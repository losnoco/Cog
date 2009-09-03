///**********************************************************************************************************************************
///  GCOneShotEffectTimer.m
///  GCDrawKit
///
///  Created by graham on 24/04/2007.
///  Released under the Creative Commons license 2007 Apptree.net.
///
/// 
///  This work is licensed under the Creative Commons Attribution-ShareAlike 2.5 License.
///  To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/2.5/ or send a letter to
///  Creative Commons, 543 Howard Street, 5th Floor, San Francisco, California, 94105, USA.
///
///**********************************************************************************************************************************

#import "GCOneShotEffectTimer.h"

@interface GCOneShotEffectTimer (Private)

- (id)		initWithTimeInterval:(NSTimeInterval) t forDelegate:(id) del;
- (void)	setDelegate:(id) del;
- (id)		delegate;
- (void)	osfx_callback:(NSTimer*) timer;

@end


@implementation GCOneShotEffectTimer

+ (id)		oneShotWithTime:(NSTimeInterval) t forDelegate:(id) del
{
	GCOneShotEffectTimer* ft = [[GCOneShotEffectTimer alloc] initWithTimeInterval:t forDelegate:del];
	
	// unlike the usual case, this is returned retained (by self, effectively). The one-shot releases
	// itself when it's complete
	
	return ft;
}


- (id)		initWithTimeInterval:(NSTimeInterval) t forDelegate:(id) del
{
	[super init];
	[self setDelegate:del];
	
	_total = t;
	_timer = [NSTimer scheduledTimerWithTimeInterval:1/60.0f target:self selector:@selector(osfx_callback:) userInfo:nil repeats:YES];
	_start = [NSDate timeIntervalSinceReferenceDate];

	return self;
}


- (void)	dealloc
{
	[_timer invalidate];
	[_delegate release];
	[super dealloc];
}


- (void)	setDelegate:(id) del
{
	// delegate is retained and released when one-shot completes. This allows some effects to work even
	// though the original delegate might be released by the caller.
	
	[del retain];
	[_delegate release];
	_delegate = del;
}


- (id)		delegate
{
	return _delegate;
}


- (void)	osfx_callback:(NSTimer*) timer
{
	NSTimeInterval elapsed = [NSDate timeIntervalSinceReferenceDate] - _start;
	float val = elapsed / _total;
	
	//NSLog(@"t = %f", val );
	
	if ( elapsed > _total )
	{
		[timer invalidate];
		_timer = nil;

		if ( _delegate && [_delegate respondsToSelector:@selector(oneShotComplete)])
			[_delegate oneShotComplete];
		
		[self release];
	}
	else
	{
		if ( _delegate && [_delegate respondsToSelector:@selector(oneShotHasReached:)])
			[_delegate oneShotHasReached:val];
	
		if ( _delegate && [_delegate respondsToSelector:@selector(oneShotHasReachedInverse:)])
			[_delegate oneShotHasReachedInverse:1.0 - val];
	}
}


@end
