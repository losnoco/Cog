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

	self.spectrumView = [[SpectrumView alloc] initWithFrame:[[self window] frame]];
	[[self window] setContentView:self.spectrumView];

	[self.spectrumView enableCameraControl];

	if(playbackController.playbackStatus == CogStatusPlaying)
		[self.spectrumView startPlayback];
}

- (IBAction)toggleWindow:(id)sender {
	if([[self window] isVisible])
		[[self window] orderOut:self];
	else
		[self showWindow:self];
}

- (IBAction)showWindow:(id)sender {
	if(self.spectrumView && playbackController.playbackStatus == CogStatusPlaying)
		[self.spectrumView startPlayback];
	return [super showWindow:sender];
}

@end
