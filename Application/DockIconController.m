//
//  DockIconController.m
//  Cog
//
//  Created by Vincent Spader on 2/28/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "DockIconController.h"
#import "PlaybackController.h"
#import <CogAudio/Status.h>

@implementation DockIconController

static NSString *DockIconPlaybackStatusObservationContext = @"DockIconPlaybackStatusObservationContext";

- (void)startObserving {
	[playbackController addObserver:self forKeyPath:@"playbackStatus" options:(NSKeyValueObservingOptionNew | NSKeyValueObservingOptionInitial) context:(__bridge void *_Nullable)(DockIconPlaybackStatusObservationContext)];
	[playbackController addObserver:self forKeyPath:@"progressBarStatus" options:(NSKeyValueObservingOptionNew | NSKeyValueObservingOptionInitial) context:(__bridge void *_Nullable)(DockIconPlaybackStatusObservationContext)];
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.colorfulDockIcons" options:0 context:(__bridge void *_Nullable)(DockIconPlaybackStatusObservationContext)];
}

- (void)stopObserving {
	[playbackController removeObserver:self forKeyPath:@"playbackStatus"];
	[playbackController removeObserver:self forKeyPath:@"progressBarStatus"];
	[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.colorfulDockIcons"];
}

static NSString *getBadgeName(NSString *baseName, BOOL colorfulIcons) {
	if(colorfulIcons) {
		return [baseName stringByAppendingString:@"Colorful"];
	} else {
		return [baseName stringByAppendingString:@"Normal"];
	}
}

- (void)refreshDockIcon:(NSInteger)playbackStatus withProgress:(double)progressStatus {
	BOOL displayChanged = NO;
	BOOL drawIcon = NO;
	BOOL removeProgress = NO;

	if(playbackStatus < 0)
		playbackStatus = lastPlaybackStatus;
	else {
		lastPlaybackStatus = playbackStatus;
		drawIcon = YES;
	}

	if(progressStatus < -2)
		progressStatus = [lastProgressStatus doubleValue];
	else {
		if(progressStatus < 0 && [lastProgressStatus doubleValue] >= 0)
			removeProgress = YES;
		lastProgressStatus = [NSNumber numberWithDouble:progressStatus];
	}

	BOOL displayProgress = (progressStatus >= 0.0);

	NSImage *badgeImage = nil;

	BOOL colorfulIcons = [[NSUserDefaults standardUserDefaults] boolForKey:@"colorfulDockIcons"];

	if((colorfulIcons && lastColorfulStatus < 1) ||
	   (!colorfulIcons && lastColorfulStatus != 0)) {
		lastColorfulStatus = colorfulIcons ? 1 : 0;
		drawIcon = YES;
	}

	NSDockTile *dockTile = [NSApp dockTile];

	if(drawIcon) {
		switch(playbackStatus) {
			case CogStatusPlaying:
				badgeImage = [NSImage imageNamed:getBadgeName(@"Play", colorfulIcons)];
				break;
			case CogStatusPaused:
				badgeImage = [NSImage imageNamed:getBadgeName(@"Pause", colorfulIcons)];
				break;

			default:
				badgeImage = [NSImage imageNamed:getBadgeName(@"Stop", colorfulIcons)];
				break;
		}

		NSSize badgeSize = [badgeImage size];

		NSImage *newDockImage = [dockImage copy];
		[newDockImage lockFocus];

		[badgeImage drawInRect:NSMakeRect(0, 0, 1024, 1024)
		              fromRect:NSMakeRect(0, 0, badgeSize.width, badgeSize.height)
		             operation:NSCompositingOperationSourceOver
		              fraction:1.0];

		[newDockImage unlockFocus];

		imageView = [[NSImageView alloc] init];
		[imageView setImage:newDockImage];
		[dockTile setContentView:imageView];

		progressIndicator = [[NSProgressIndicator alloc] initWithFrame:NSMakeRect(0.0, 0.0, dockTile.size.width, 10.0)];
		[progressIndicator setStyle:NSProgressIndicatorBarStyle];
		[progressIndicator setIndeterminate:NO];
		[progressIndicator setBezeled:YES];
		[progressIndicator setMinValue:0];
		[progressIndicator setMaxValue:100];
		[progressIndicator setHidden:YES];

		[imageView addSubview:progressIndicator];

		displayChanged = YES;
	}

	if(displayProgress) {
		if(!imageView) {
			imageView = [[NSImageView alloc] init];
			[imageView setImage:[NSApp applicationIconImage]];
			[dockTile setContentView:imageView];
		}

		if(!progressIndicator) {
			progressIndicator = [[NSProgressIndicator alloc] initWithFrame:NSMakeRect(0.0, 0.0, dockTile.size.width, 10.0)];
			[progressIndicator setIndeterminate:NO];
			[progressIndicator setBezeled:YES];
			[progressIndicator setMinValue:0];
			[progressIndicator setMaxValue:100];

			[imageView addSubview:progressIndicator];
		}

		[progressIndicator setDoubleValue:progressStatus];
		[progressIndicator setHidden:NO];

		displayChanged = YES;
	}

	if(removeProgress) {
		if(progressIndicator)
			[progressIndicator setHidden:YES];

		displayChanged = YES;
	}

	if(displayChanged)
		[dockTile display];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
	if([DockIconPlaybackStatusObservationContext isEqual:(__bridge id)(context)]) {
		if([keyPath isEqualToString:@"playbackStatus"]) {
			NSInteger playbackStatus = [[change objectForKey:NSKeyValueChangeNewKey] integerValue];

			[self refreshDockIcon:playbackStatus withProgress:-10];
		} else if([keyPath isEqualToString:@"progressBarStatus"]) {
			double progressStatus = [[change objectForKey:NSKeyValueChangeNewKey] doubleValue];

			[self refreshDockIcon:-1 withProgress:progressStatus];
		} else if([keyPath isEqualToString:@"values.colorfulDockIcons"]) {
			[self refreshDockIcon:-1 withProgress:-10];
		}
	} else {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
	}
}

- (void)awakeFromNib {
	dockImage = [[NSImage imageNamed:@"Plaque"] copy];
	lastColorfulStatus = -1;
	lastProgressStatus = [NSNumber numberWithDouble:-1];
	imageView = nil;
	progressIndicator = nil;
	[self startObserving];
}

- (void)dealloc {
	[self stopObserving];
}

@end
