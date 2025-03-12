//
//  SpectrumWindowController.m
//  Cog
//
//  Created by Christopher Snowhill on 5/22/22.
//

#import "SpectrumWindowController.h"

#import "SpectrumViewSK.h"
#import "SpectrumViewCG.h"
#import "ProjectMView.h"

static void *kSpectrumWindowContext = &kSpectrumWindowContext;

@interface SpectrumWindowController ()
@property SpectrumViewSK *spectrumViewSK;
@property SpectrumViewCG *spectrumViewCG;
@property ProjectMView *projectMView;
@end

@implementation SpectrumWindowController

- (id)init {
	self = [super initWithWindowNibName:@"SpectrumWindow"];
	if(self) {
		[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.spectrumSceneKit" options:0 context:kSpectrumWindowContext];
	}
	return self;
}

- (void)windowDidLoad {
	[super windowDidLoad];
}

- (void)dealloc {
	[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.spectrumSceneKit" context:kSpectrumWindowContext];
}

- (void)observeValueForKeyPath:(NSString *)keyPath
					  ofObject:(id)object
						change:(NSDictionary<NSKeyValueChangeKey, id> *)change
					   context:(void *)context {
	if(context == kSpectrumWindowContext) {
		if([keyPath isEqualToString:@"values.spectrumSceneKit"]) {
			self.spectrumViewSK = nil;
			self.spectrumViewCG = nil;
			[self startRunning];
		}
	} else {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
	}
}

- (void)startRunning {
	if(!self.projectMView) {
		NSRect frame = [[self window] frame];
		self.projectMView = [[ProjectMView alloc] initWithFrame:frame];
		if(self.projectMView) {
			[[self window] setContentView:self.projectMView];
		}
	}
#if 0
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
#endif
}

- (void)stopRunning {
	[[self window] setContentView:nil];
	self.spectrumViewSK = nil;
	self.spectrumViewCG = nil;
	self.projectMView = nil;
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
