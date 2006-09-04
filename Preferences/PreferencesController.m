//
//  PreferencesController.m
//  Cog
//
//  Created by Vincent Spader on 9/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "PreferencesController.h"


@implementation PreferencesController

- (IBAction)showPrefs:(id)sender
{
    if (!prefs) {
        // Determine path to the sample preference panes
        NSString *pathToPanes = [[NSBundle mainBundle] resourcePath];
		NSLog(@"Loading preferences...%@", pathToPanes);
        
        prefs = [[SS_PrefsController alloc] initWithPanesSearchPath:pathToPanes bundleExtension:@"preferencePane"];
		[prefs setDebug:YES];
		
        // Set which panes are included, and their order.
//        [prefs setPanesOrder:[NSArray arrayWithObjects:@"General", @"Updating", @"A Non-Existent Preference Pane", nil]];
    }
    
    // Show the preferences window.
    [prefs showPreferencesWindow];
}


- (void)dealloc
{
    [prefs release];
	
	[super dealloc];
}

@end
