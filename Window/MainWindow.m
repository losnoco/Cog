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

void showCrashlyticsConsent(NSWindow *window) {
	BOOL askedConsent = [[NSUserDefaults standardUserDefaults] boolForKey:@"crashlyticsAskedConsent"];
	if(!askedConsent) {
		[[NSUserDefaults standardUserDefaults] registerDefaults:@{ @"rubberbandEngine": @"disabled" }];

		[window orderFront:window];

		NSAlert *alert = [[NSAlert alloc] init];
		[alert setMessageText:NSLocalizedString(@"CrashlyticsConsentTitle", @"")];
		[alert setInformativeText:NSLocalizedString(@"CrashlyticsConsentText", @"")];
		[alert addButtonWithTitle:NSLocalizedString(@"ConsentYes",@"")];
		[alert addButtonWithTitle:NSLocalizedString(@"ConsentNo", @"")];
		
		[alert beginSheetModalForWindow:window completionHandler:^(NSModalResponse returnCode) {
			if(returnCode == NSAlertFirstButtonReturn) {
				[[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"crashlyticsConsented"];
			}
		}];
		
		[[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"crashlyticsAskedConsent"];
	} else {
		[[NSUserDefaults standardUserDefaults] registerDefaults:@{ @"rubberbandEngine": @"faster" }];
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

	hdcdLogo = [NSImage imageNamed:@"hdcdLogoTemplate"];

	[self showHDCDLogo:NO];
	
	if(![[NSUserDefaults standardUserDefaults] boolForKey:@"miniMode"]) {
		showCrashlyticsConsent(self);
	}
}

- (void)showHDCDLogo:(BOOL)show {
	for(NSToolbarItem* toolbarItem in [mainToolbar items]) {
		if([[toolbarItem itemIdentifier] isEqualToString:@"hdcdMain"]) {
			if(show)
				[toolbarItem setImage:hdcdLogo];
			else
				[toolbarItem setImage:nil];
		}
	}
}

@end
