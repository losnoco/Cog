//
//  SCVisWindowController.m
//  Cog
//
//  Created by Christopher Snowhill on 9/25/25.
//

#import <Cocoa/Cocoa.h>

#import <MetalKit/MetalKit.h>

#import "SCVisWindowController.h"

#import "SCView.h"

static void *kSCVisWindowContext = &kSCVisWindowContext;

@interface SCVisWindowController ()
@property SCView *sc55View;
@end

@implementation SCVisWindowController

- (id)init {
	self = [super initWithWindowNibName:@"SCVisWindow"];
	return self;
}

- (void)windowDidLoad {
	[super windowDidLoad];

	[self startRunning];
}

- (void)dealloc {
}

- (void)startRunning {
	if(!self.sc55View) {
		self.sc55View = [[SCView alloc] init];
		if(self.sc55View) {
			NSWindow *window = self.window;
			NSRect contentRect = self.sc55View.bounds;
			NSRect frameRect = window.frame;
			NSRect originalContentRect = [window contentRectForFrameRect:frameRect];
			contentRect.origin = originalContentRect.origin;
			NSRect windowFrame = [[self window] frameRectForContentRect:contentRect];
			[window setFrame:windowFrame display:YES animate:YES];
			[window setMinSize:windowFrame.size];
			[window setMaxSize:windowFrame.size];

			[window setContentView:self.sc55View];
		}
	}

	if(playbackController.playbackStatus == CogStatusPlaying) {
		if(self.sc55View)
			[self.sc55View startPlayback];
	}
}

- (void)stopRunning {
	[[self window] setContentView:nil];
	self.sc55View = nil;
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

