//
//  PreferencesController.h
//  Cog
//
//  Created by Vincent Spader on 9/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "SS_PrefsController.h"

@interface PreferencesController : NSObject {
    SS_PrefsController *prefs;
}

- (IBAction)showPrefs:(id)sender;

@end
