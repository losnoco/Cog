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

static NSString *CogCustomDockIconsReloadNotification = @"CogCustomDockIconsReloadNotification";

- (void)startObserving {
	[playbackController addObserver:self forKeyPath:@"playbackStatus" options:(NSKeyValueObservingOptionNew | NSKeyValueObservingOptionInitial) context:(__bridge void *_Nullable)(DockIconPlaybackStatusObservationContext)];
	[playbackController addObserver:self forKeyPath:@"progressOverall" options:(NSKeyValueObservingOptionNew | NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionOld) context:(__bridge void *_Nullable)(DockIconPlaybackStatusObservationContext)];
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.colorfulDockIcons" options:0 context:(__bridge void *_Nullable)(DockIconPlaybackStatusObservationContext)];
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.customDockIcons" options:0 context:(__bridge void *_Nullable)(DockIconPlaybackStatusObservationContext)];
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.customDockIconsPlaque" options:0 context:(__bridge void *_Nullable)(DockIconPlaybackStatusObservationContext)];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(refreshDockIcons:) name:CogCustomDockIconsReloadNotification object:nil];
}

- (void)stopObserving {
	[playbackController removeObserver:self forKeyPath:@"playbackStatus" context:(__bridge void *_Nullable)(DockIconPlaybackStatusObservationContext)];
	[playbackController removeObserver:self forKeyPath:@"progressOverall" context:(__bridge void *_Nullable)(DockIconPlaybackStatusObservationContext)];
	[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.colorfulDockIcons" context:(__bridge void *_Nullable)(DockIconPlaybackStatusObservationContext)];
	[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.customDockIcons" context:(__bridge void *_Nullable)(DockIconPlaybackStatusObservationContext)];
	[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.customDockIconsPlaque" context:(__bridge void *_Nullable)(DockIconPlaybackStatusObservationContext)];
	[[NSNotificationCenter defaultCenter] removeObserver:self name:CogCustomDockIconsReloadNotification object:nil];
}

- (void)startObservingProgress:(NSProgress *)progress {
	[progress addObserver:self forKeyPath:@"fractionCompleted" options:0 context:(__bridge void *_Nullable)(DockIconPlaybackStatusObservationContext)];
}

- (void)stopObservingProgress:(NSProgress *)progress {
	[progress removeObserver:self forKeyPath:@"fractionCompleted" context:(__bridge void *_Nullable)(DockIconPlaybackStatusObservationContext)];
}

static NSString *getBadgeName(NSString *baseName, BOOL colorfulIcons) {
	if(@available(macOS 26, *)) {
		return baseName;
	} else {
		if(colorfulIcons) {
			return [baseName stringByAppendingString:@"Colorful"];
		} else {
			return [baseName stringByAppendingString:@"Normal"];
		}
	}
}

static NSString *getCustomIconName(NSString *baseName) {
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
	NSString *basePath = [[paths firstObject] stringByAppendingPathComponent:@"Cog"];
	basePath = [basePath stringByAppendingPathComponent:@"Icons"];
	basePath = [basePath stringByAppendingPathComponent:baseName];
	return [basePath stringByAppendingPathExtension:@"png"];
}

- (BOOL)loadCustomDockIcons {
	NSError *error = nil;
	NSData *dataStopIcon = [NSData dataWithContentsOfFile:getCustomIconName(@"Stop") options:(NSDataReadingMappedIfSafe) error:&error];
	if(!dataStopIcon || error) {
		return NO;
	}
	NSData *dataPlayIcon = [NSData dataWithContentsOfFile:getCustomIconName(@"Play") options:(NSDataReadingMappedIfSafe) error:&error];
	if(!dataPlayIcon || error) {
		return NO;
	}
	NSData *dataPauseIcon = [NSData dataWithContentsOfFile:getCustomIconName(@"Pause") options:(NSDataReadingMappedIfSafe) error:&error];
	if(!dataPauseIcon || error) {
		return NO;
	}
	dockCustomStop = [[NSImage alloc] initWithData:dataStopIcon];
	dockCustomPlay = [[NSImage alloc] initWithData:dataPlayIcon];
	dockCustomPause = [[NSImage alloc] initWithData:dataPauseIcon];
	return (dockCustomStop && dockCustomPlay && dockCustomPause);
}

- (void)refreshDockIcon:(NSInteger)playbackStatus withProgress:(double)progressStatus {
	// Really weird crash user experienced because the plaque image didn't load?
	if(!dockImage || dockImage.size.width == 0 || dockImage.size.height == 0) return;

	BOOL displayChanged = NO;
	BOOL drawIcon = NO;
	BOOL removeProgress = NO;

	BOOL useCustomDockIcons = [[NSUserDefaults standardUserDefaults] boolForKey:@"customDockIcons"];
	BOOL useCustomDockIconsPlaque = [[NSUserDefaults standardUserDefaults] boolForKey:@"customDockIconsPlaque"];

	if(useCustomDockIcons && !dockCustomLoaded) {
		dockCustomLoaded = [self loadCustomDockIcons];
		if(!dockCustomLoaded) {
			useCustomDockIcons = NO;
		}
	}

	if(useCustomDockIcons != lastDockCustom ||
	   useCustomDockIconsPlaque != lastDockCustomPlaque) {
		lastDockCustom = useCustomDockIcons;
		lastDockCustomPlaque = useCustomDockIconsPlaque;
		drawIcon = YES;

		if(!useCustomDockIcons) {
			dockCustomLoaded = NO;
			dockCustomStop = nil;
			dockCustomPlay = nil;
			dockCustomPause = nil;
		}
	}

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
		lastProgressStatus = @(progressStatus);
	}

	BOOL errorStatus = [playbackController isError];

	BOOL displayProgress = (progressStatus >= 0.0);

	NSImage *badgeImage = nil;

	BOOL colorfulIcons = [[NSUserDefaults standardUserDefaults] boolForKey:@"colorfulDockIcons"];
	BOOL glassIcons = NO;
	if(@available(macOS 26, *)) {
		colorfulIcons = NO;
		glassIcons = YES;
	}

	if((colorfulIcons && lastColorfulStatus < 1) ||
	   (!colorfulIcons && lastColorfulStatus != 0)) {
		lastColorfulStatus = colorfulIcons ? 1 : 0;
		drawIcon = YES;
	}

	NSDockTile *dockTile = [NSApp dockTile];

	if(drawIcon) {
		if(glassIcons && errorStatus) {
			badgeImage = [NSImage imageNamed:@"Huh"];
		} else {
			switch(playbackStatus) {
				case CogStatusPlaying:
					badgeImage = useCustomDockIcons ? dockCustomPlay : [NSImage imageNamed:getBadgeName(@"Play", colorfulIcons)];
					break;
				case CogStatusPaused:
					badgeImage = useCustomDockIcons ? dockCustomPause : [NSImage imageNamed:getBadgeName(@"Pause", colorfulIcons)];
					break;

				default:
					badgeImage = useCustomDockIcons ? dockCustomStop : [NSImage imageNamed:getBadgeName(@"Stop", colorfulIcons)];
					break;
			}
		}

		NSSize badgeSize = [badgeImage size];

		NSImage *newDockImage = (glassIcons || (useCustomDockIcons && !useCustomDockIconsPlaque)) ? [[NSImage alloc] initWithSize:NSMakeSize(1024, 1024)] : [dockImage copy];
		[newDockImage lockFocus];

		[badgeImage drawInRect:NSMakeRect(0, 0, 1024, 1024)
		              fromRect:NSMakeRect(0, 0, badgeSize.width, badgeSize.height)
		             operation:NSCompositingOperationSourceOver
		              fraction:1.0];

		[newDockImage unlockFocus];

		imageView = [NSImageView new];
		[imageView setImage:newDockImage];
		[dockTile setContentView:imageView];

		progressIndicator = [[NSProgressIndicator alloc] initWithFrame:NSMakeRect(0.0, 0.0, dockTile.size.width, 10.0)];
		[progressIndicator setStyle:NSProgressIndicatorStyleBar];
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
			imageView = [NSImageView new];
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
		} else if([keyPath isEqualToString:@"progressOverall"]) {
			double progressStatus = [lastProgressStatus doubleValue];

			id objNew = [change objectForKey:NSKeyValueChangeNewKey];
			id objOld = [change objectForKey:NSKeyValueChangeOldKey];

			NSProgress *progressNew = nil, *progressOld = nil;

			if(objNew && [objNew isKindOfClass:[NSProgress class]])
				progressNew = (NSProgress *)objNew;
			if(objOld && [objOld isKindOfClass:[NSProgress class]])
				progressOld = (NSProgress *)objOld;

			if(progressOld) {
				[self stopObservingProgress:progressOld];
				progressStatus = -1;
			}

			if(progressNew) {
				[self startObservingProgress:progressNew];
				progressStatus = progressNew.fractionCompleted * 100.0;
			}

			[self refreshDockIcon:-1 withProgress:progressStatus];
		} else if([keyPath isEqualToString:@"values.colorfulDockIcons"] ||
				  [keyPath isEqualToString:@"values.customDockIcons"] ||
				  [keyPath isEqualToString:@"values.customDockIconsPlaque"]) {
			[self refreshDockIcon:-1 withProgress:-10];
		} else if([keyPath isEqualToString:@"fractionCompleted"]) {
			double progressStatus = [(NSProgress *)object fractionCompleted];
			[self refreshDockIcon:-1 withProgress:progressStatus * 100.0];
		}
	} else {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
	}
}

- (void)refreshDockIcons:(NSNotification *)notification {
	lastDockCustom = NO;
	dockCustomLoaded = NO;
	[self refreshDockIcon:-1 withProgress:-10];
}

- (void)awakeFromNib {
	dockImage = [[NSImage imageNamed:@"Plaque"] copy];
	lastColorfulStatus = -1;
	lastProgressStatus = @(-1.0);
	imageView = nil;
	progressIndicator = nil;
	[self startObserving];
}

- (void)dealloc {
	[self stopObserving];
}

@end
