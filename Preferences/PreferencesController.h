//
//  PreferencesController.h
//  Cog
//
//  Created by Vincent Spader on 9/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class PreferencesWindow;
@interface PreferencesController : NSObject {
	PreferencesWindow *window;
}

- (IBAction)showPreferences:(id)sender;

@end
