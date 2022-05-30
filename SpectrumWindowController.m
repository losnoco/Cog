//
//  SpectrumWindowController.m
//  Cog
//
//  Created by Christopher Snowhill on 5/22/22.
//

#import "SpectrumWindowController.h"

#import "SpectrumView.h"

@interface SpectrumWindowController ()
@property SpectrumView *spectrumView;
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
	if(!self.spectrumView) {
		self.spectrumView = [[SpectrumView alloc] initWithFrame:[[self window] frame]];
		[[self window] setContentView:self.spectrumView];
		if(!self.spectrumView) return;

		[self.spectrumView enableCameraControl];
	}

	if(playbackController.playbackStatus == CogStatusPlaying)
		[self.spectrumView startPlayback];
}

- (void)stopRunning {
	[[self window] setContentView:nil];
	self.spectrumView = nil;
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
