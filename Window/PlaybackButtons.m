//
//  PlaybackButtons.m
//  Cog
//
//  Created by Vincent Spader on 2/28/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "PlaybackButtons.h"
#import "PlaybackController.h"

#import <CogAudio/Status.h>

#import "Logging.h"

@implementation PlaybackButtons

static NSString *PlaybackButtonsPlaybackStatusObservationContext = @"PlaybackButtonsPlaybackStatusObservationContext";

- (void)dealloc
{
	[self stopObserving];
}

- (void)awakeFromNib
{
	[self startObserving];
}

- (void)startObserving
{
	[playbackController addObserver:self forKeyPath:@"playbackStatus" options:(NSKeyValueObservingOptionNew | NSKeyValueObservingOptionInitial) context:(__bridge void * _Nullable)(PlaybackButtonsPlaybackStatusObservationContext)];
    [[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.enableStopButton" options:(NSKeyValueObservingOptionNew | NSKeyValueObservingOptionInitial) context:(__bridge void * _Nullable)(PlaybackButtonsPlaybackStatusObservationContext)];
}

- (void)stopObserving
{
	[playbackController removeObserver:self forKeyPath:@"playbackStatus"];
    [[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.enableStopButton"];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
	if ([PlaybackButtonsPlaybackStatusObservationContext isEqual:(__bridge id)(context)])
	{
        if ([keyPath isEqualToString:@"playbackStatus"])
        {
            NSInteger playbackStatus = [[change objectForKey:NSKeyValueChangeNewKey] integerValue];

            NSImage *image = nil;

            if (playbackStatus == CogStatusPlaying) {
                image = [NSImage imageNamed:@"pauseTemplate"];
            }
            else {
                image = [NSImage imageNamed:@"playTemplate"];
            }
		
            [self setImage:image forSegment:1];
        }
        else if ([keyPath isEqualToString:@"values.enableStopButton"])
        {
            BOOL segmentEnabled = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] boolForKey:@"enableStopButton"];
            if (segmentEnabled) {
                [self setSegmentCount:4];
                [self setImage:[NSImage imageNamed:@"stopTemplate"] forSegment:2];
                [self setImage:[NSImage imageNamed:@"nextTemplate"] forSegment:3];
            }
            else {
                [self setSegmentCount:3];
                [self setImage:[NSImage imageNamed:@"nextTemplate"] forSegment:2];
            }
        }
	}
	else
	{
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
	}
}

- (BOOL)sendAction:(SEL)theAction to:(id)theTarget
{
	DLog(@"Mouse down!");
    
    int clickedSegment = (int) [self selectedSegment];
    int segmentCount = (int) [self segmentCount];
    if (segmentCount == 3 && clickedSegment == 2)
        clickedSegment = 3;
	
	if (clickedSegment == 0) //Previous
	{
		[playbackController prev:self];
	}
	else if (clickedSegment == 1) //Play
	{
		[playbackController playPauseResume:self];
	}
    else if (clickedSegment == 2) //Stop
    {
        [playbackController stop:self];
    }
	else if (clickedSegment == 3) //Next
	{
		[playbackController next:self];
	}
	else {
		return NO;
	}
	
	return YES;
	
}
			

@end
