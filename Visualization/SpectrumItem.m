//
//  SpectrumItem.m
//  Cog
//
//  Created by Christopher Snowhill on 2/13/22.
//

#import "SpectrumItem.h"

#import "SpectrumViewSK.h"
#import "SpectrumViewCG.h"

#import "Logging.h"

static void *kSpectrumItemContext = &kSpectrumItemContext;

// Space either side of the spectrum, so it doesn't touch its neighbours.
static const CGFloat kSpectrumItemPadding = 4.0;

/// Holds the spectrum, padded, and takes its clicks: the item opens the
/// spectrum window.
@interface SpectrumItemView : NSView
@property(nonatomic, copy) void (^onClick)(void);
@end

@implementation SpectrumItemView

- (NSView *)hitTest:(NSPoint)point {
	return NSPointInRect(point, self.frame) ? self : nil;
}

- (void)mouseDown:(NSEvent *)event {
	if(self.onClick) self.onClick();
}

@end

@interface SpectrumItem ()
@property SpectrumViewSK *spectrumViewSK;
@property SpectrumViewCG *spectrumViewCG;
@end

@implementation SpectrumItem

- (void)awakeFromNib {
	[self startRunning];

	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.spectrumSceneKit" options:0 context:kSpectrumItemContext];
}

- (void)startRunning {
	if(!self.spectrumViewSK && !self.spectrumViewCG) {
		NSRect frame = NSMakeRect(0, 0, 64, 26);
		self.spectrumViewSK = [SpectrumViewSK createGuardWithFrame:frame];
		if(self.spectrumViewSK) {
			[self setPaddedView:self.spectrumViewSK];
		} else {
			self.spectrumViewCG = [[SpectrumViewCG alloc] initWithFrame:frame];
			if(self.spectrumViewCG) {
				[self setPaddedView:self.spectrumViewCG];
			}
		}
	}

	if(playbackController.playbackStatus == CogStatusPlaying) {
		if(self.spectrumViewSK)
			[self.spectrumViewSK startPlayback];
		else if(self.spectrumViewCG)
			[self.spectrumViewCG startPlayback];
	}
}

- (void)setPaddedView:(NSView *)view {
	NSRect frame = view.frame;
	SpectrumItemView *container = [[SpectrumItemView alloc] initWithFrame:NSMakeRect(0, 0, NSWidth(frame) + 2 * kSpectrumItemPadding, NSHeight(frame))];
	view.frame = NSOffsetRect(frame, kSpectrumItemPadding, 0);
	[container addSubview:view];
	container.toolTip = NSLocalizedString(@"Spectrum. Click to open the spectrum.", @"Spectrum toolbar item tooltip");
	__weak SpectrumItem *weakSelf = self;
	container.onClick = ^{
		SpectrumItem *item = weakSelf;
		if(item) [item->windowController showWindow:nil];
	};
	[self setView:container];
}

- (void)dealloc {
	[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.spectrumSceneKit" context:kSpectrumItemContext];
}

- (void)observeValueForKeyPath:(NSString *)keyPath
					  ofObject:(id)object
						change:(NSDictionary<NSKeyValueChangeKey, id> *)change
					   context:(void *)context {
	if(context == kSpectrumItemContext) {
		if([keyPath isEqualToString:@"values.spectrumSceneKit"]) {
			self.spectrumViewSK = nil;
			self.spectrumViewCG = nil;
			[self startRunning];
		}
	} else {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
	}
}

@end
