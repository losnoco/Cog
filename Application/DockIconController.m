//
//  DockIconController.m
//  Cog
//
//  Created by Vincent Spader on 2/28/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "DockIconController.h"
#import <CogAudio/Status.h>
#import "PlaybackController.h"

@implementation DockIconController

static NSString *DockIconPlaybackStatusObservationContext = @"DockIconPlaybackStatusObservationContext";

- (void)startObserving
{
	[playbackController addObserver:self forKeyPath:@"playbackStatus" options:(NSKeyValueObservingOptionNew | NSKeyValueObservingOptionInitial) context:(__bridge void * _Nullable)(DockIconPlaybackStatusObservationContext)];
    [[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.colorfulDockIcons"		options:0 context:(__bridge void * _Nullable)(DockIconPlaybackStatusObservationContext)];
}

- (void)stopObserving
{
	[playbackController removeObserver:self forKeyPath:@"playbackStatus"];
    [[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.colorfulDockIcons"];
}

static NSString *getBadgeName(NSString *baseName, BOOL colorfulIcons)
{
    if (colorfulIcons)
    {
        return [baseName stringByAppendingString:@"Colorful"];
    }
    else
    {
        return baseName;
    }
}

- (void)refreshDockIcon:(NSInteger)playbackStatus
{
    if ( playbackStatus < 0 )
        playbackStatus = lastPlaybackStatus;
    else
        lastPlaybackStatus = playbackStatus;
    
    NSImage *badgeImage = nil;
    
    BOOL colorfulIcons = [[NSUserDefaults standardUserDefaults] boolForKey:@"colorfulDockIcons"];
    
    if (playbackStatus == kCogStatusPlaying) {
        badgeImage = [NSImage imageNamed:getBadgeName(@"playDockBadge", colorfulIcons)];
    }
    else if (playbackStatus == kCogStatusPaused) {
        badgeImage = [NSImage imageNamed:getBadgeName(@"pauseDockBadge", colorfulIcons)];
    }
    else {
        badgeImage = [NSImage imageNamed:getBadgeName(@"stopDockBadge", colorfulIcons)];
    }
    
    NSSize badgeSize = [badgeImage size];
    
    NSImage *newDockImage = [dockImage copy];
    [newDockImage lockFocus];
    
    [badgeImage drawInRect:NSMakeRect(0, 0, 128, 128)
                  fromRect:NSMakeRect(0, 0, badgeSize.width, badgeSize.height)
                 operation:NSCompositeSourceOver fraction:1.0];
    
    [newDockImage unlockFocus];
    [NSApp setApplicationIconImage:newDockImage];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
	if ([DockIconPlaybackStatusObservationContext isEqual:(__bridge id)(context)])
	{
        if ([keyPath isEqualToString:@"playbackStatus"])
        {
            NSInteger playbackStatus = [[change objectForKey:NSKeyValueChangeNewKey] integerValue];

            [self refreshDockIcon:playbackStatus];
        }
        else if ([keyPath isEqualToString:@"values.colorfulDockIcons"])
        {
            [self refreshDockIcon:-1];
        }
	}
	else
	{
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
	}
}

- (void)awakeFromNib
{
	dockImage = [[NSImage imageNamed:@"icon_blank"] copy];
	[self startObserving];
}

- (void)dealloc
{
	[self stopObserving];
}

@end
