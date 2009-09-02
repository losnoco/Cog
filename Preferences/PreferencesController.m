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

- (IBAction)showPreferences:(id)sender
{
    if (nil == window) {
        // Determine path to the sample preference panes
        NSString *pluginPath = [[NSBundle mainBundle] pathForResource:@"General" ofType:@"preferencePane"];
		NSBundle *bundle = [NSBundle bundleWithPath:pluginPath];

        
		PreferencePluginController *pluginController = [[PreferencePluginController alloc] initWithPlugins:[NSArray arrayWithObject:bundle]];
		
		window = [[PreferencesWindow alloc] initWithPreferencePanes:[pluginController preferencePanes]];
		
        // Set which panes are included, and their order.
        //[prefs setPanesOrder:[NSArray arrayWithObjects:@"General", @"Updating", @"A Non-Existent Preference Pane", nil]];
		[pluginController release];
    }
    
    // Show the preferences window.
    [window show];
}


- (void)dealloc
{
    [window release];
	
	[super dealloc];
}

@end
