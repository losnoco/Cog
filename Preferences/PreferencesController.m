//
//  PreferencesController.m
//  Cog
//
//  Created by Vincent Spader on 9/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "PreferencesController.h"
#import "Cog-Swift.h"

@implementation PreferencesController

- (void)initWindow {
	if(nil == window) {
		window = [PreferencesWindow new];
	}
}

- (IBAction)showPreferences:(id)sender {
	[self initWindow];

	// Show the preferences window.
	[window show];
}

- (IBAction)showPathSuggester:(id)sender {
	[self initWindow];

	// Show the path suggester
	[window showPathSuggester];
}

- (IBAction)showRubberbandSettings:(id)sender {
	[self initWindow];

	[window showRubberbandSettings];
}

@end
