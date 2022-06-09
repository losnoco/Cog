//
//  SpectrumWindowController.m
//  Cog
//
//  Created by Christopher Snowhill on 5/22/22.
//

#import "SpectrumWindowController.h"

#import "SpectrumView.h"
#import "SpectrumViewLegacy.h"

@interface SpectrumWindowController ()
@property SpectrumView *spectrumView;
@property SpectrumViewLegacy *spectrumViewLegacy;
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
	if(!self.spectrumView && !self.spectrumViewLegacy) {
		NSRect frame = [[self window] frame];
		self.spectrumView = [[SpectrumView alloc] initWithFrame:frame];
		if(self.spectrumView) {
			[[self window] setContentView:self.spectrumView];

			[self.spectrumView enableCameraControl];
		} else {
			self.spectrumViewLegacy = [[SpectrumViewLegacy alloc] initWithFrame:frame];
			[[self window] setContentView:self.spectrumViewLegacy];
		}
	}

	if(playbackController.playbackStatus == CogStatusPlaying) {
		if(self.spectrumView)
			[self.spectrumView startPlayback];
		else if(self.spectrumViewLegacy)
			[self.spectrumViewLegacy startPlayback];
	}
}

- (void)stopRunning {
	[[self window] setContentView:nil];
	self.spectrumView = nil;
	self.spectrumViewLegacy = nil;
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
