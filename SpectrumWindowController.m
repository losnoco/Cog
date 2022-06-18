//
//  SpectrumWindowController.m
//  Cog
//
//  Created by Christopher Snowhill on 5/22/22.
//

#import "SpectrumWindowController.h"

#import "SpectrumViewSK.h"
#import "SpectrumViewCG.h"

@interface SpectrumWindowController ()
@property SpectrumViewSK *spectrumViewSK;
@property SpectrumViewCG *spectrumViewCG;
@end

@implementation SpectrumWindowController

- (id)init {
	return [super initWithWindowNibName:@"SpectrumWindow"];
}

- (void)windowDidLoad {
	[super windowDidLoad];

	[self startRunning];
}

- (void)startRunning {
	if(!self.spectrumViewSK && !self.spectrumViewCG) {
		NSRect frame = [[self window] frame];
		self.spectrumViewSK = [SpectrumViewSK createGuardWithFrame:frame];
		if(self.spectrumViewSK) {
			[[self window] setContentView:self.spectrumViewSK];

			[self.spectrumViewSK enableCameraControl];
		} else {
			self.spectrumViewCG = [[SpectrumViewCG alloc] initWithFrame:frame];
			if(self.spectrumViewCG) {
				[[self window] setContentView:self.spectrumViewCG];

				[self.spectrumViewCG enableFullView];
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

- (void)stopRunning {
	[[self window] setContentView:nil];
	self.spectrumViewSK = nil;
	self.spectrumViewCG = nil;
}

- (void)windowWillClose:(NSNotification *)notification {
	NSWindow *currentWindow = notification.object;
	if([currentWindow isEqualTo:self.window]) {
		[self stopRunning];
	}
}

- (IBAction)toggleWindow:(id)sender {
	if([[self window] isVisible])
		[[self window] orderOut:self];
	else
		[self showWindow:self];
}

- (IBAction)showWindow:(id)sender {
	[self startRunning];
	return [super showWindow:sender];
}

@end
