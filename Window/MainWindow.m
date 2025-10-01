//
//  MainWindow.m
//  Cog
//
//  Created by Vincent Spader on 2/22/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "MainWindow.h"

#import "AppController.h"

// NOTICE! We bury first time defaults that should depend on whether the install is fresh or not here
// so that they get created correctly depending on the situation.

// For instance, for the first option to get this treatment, we want time stretching to stay enabled
// for existing installations, but disable itself by default on new installs, to spare processing.

void showSentryConsent(NSWindow *window) {
	BOOL askedConsent = [[NSUserDefaults standardUserDefaults] boolForKey:@"sentryAskedConsent"];
	if(!askedConsent) {
		[window orderFront:window];

		NSAlert *alert = [[NSAlert alloc] init];
		[alert setMessageText:NSLocalizedString(@"SentryConsentTitle", @"")];
		[alert setInformativeText:NSLocalizedString(@"SentryConsentText", @"")];
		[alert addButtonWithTitle:NSLocalizedString(@"ConsentYes",@"")];
		[alert addButtonWithTitle:NSLocalizedString(@"ConsentNo", @"")];
		
		[alert beginSheetModalForWindow:window completionHandler:^(NSModalResponse returnCode) {
			if(returnCode == NSAlertFirstButtonReturn) {
				[[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"sentryConsented"];
			}
		}];
		
		[[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"sentryAskedConsent"];
	}
}

@implementation MainWindow

- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSWindowStyleMask)windowStyle backing:(NSBackingStoreType)bufferingType defer:(BOOL)deferCreation {
	self = [super initWithContentRect:contentRect styleMask:windowStyle backing:bufferingType defer:deferCreation];
	if(self) {
		[self setExcludedFromWindowsMenu:YES];
		[self setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
	}
	return self;
}

- (void)awakeFromNib {
	[super awakeFromNib];

	[playlistView setNextResponder:self];

	if(![[NSUserDefaults standardUserDefaults] boolForKey:@"miniMode"]) {
		showSentryConsent(self);
	}
}

- (void)focusSearch:(id)sender {
	[self makeFirstResponder:searchField];
	NSRange range = NSMakeRange(0, searchField.stringValue.length);
	NSText *editor = searchField.currentEditor;
	if(editor) {
		editor.selectedRange = range;
	}
}

- (IBAction)openSearch:(id)sender {
	[self focusSearch:sender];
	// hack
	NSTimer *timer = [NSTimer scheduledTimerWithTimeInterval:0.125
													  target:self
													selector:@selector(focusSearch:)
													userInfo:nil
													 repeats:NO];
	[[NSRunLoop mainRunLoop] addTimer:timer forMode:NSRunLoopCommonModes];
}

@end
