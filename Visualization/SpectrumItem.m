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
			[self setView:self.spectrumViewSK];
		} else {
			self.spectrumViewCG = [[SpectrumViewCG alloc] initWithFrame:frame];
			if(self.spectrumViewCG) {
				[self setView:self.spectrumViewCG];
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
