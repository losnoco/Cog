//
//  PreferencesController.m
//  Cog
//
//  Created by Vincent Spader on 9/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "PreferencesController.h"
#import "PreferencePluginController.h"
#import "PreferencesWindow.h"

@implementation PreferencesController

- (void)initWindow {
	if(nil == window) {
		// Determine path to the sample preference panes
		NSString *pluginPath = [[NSBundle mainBundle] pathForResource:@"Preferences" ofType:@"preferencePane"];
		NSBundle *bundle = [NSBundle bundleWithPath:pluginPath];

		PreferencePluginController *pluginController = [[PreferencePluginController alloc] initWithPlugins:@[bundle]];

		window = [[PreferencesWindow alloc] initWithPreferencePanes:[pluginController preferencePanes]];
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
