//
//  PreferencesController.h
//  Cog
//
//  Created by Zaphod Beeblebrox on 9/4/06.
//  Copyright 2006 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "SS_PrefsController.h"

@interface PreferencesController : NSObject {
    SS_PrefsController *prefs;
}

- (IBAction)showPrefs:(id)sender;

@end
