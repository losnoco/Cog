//
//  AppearancePane.h
//  General
//
//  Created by Christopher Snowhill on 11/24/24.
//
//

#import "GeneralPreferencePane.h"
#import <Cocoa/Cocoa.h>

@interface AppearancePane : GeneralPreferencePane {
}

- (IBAction)setDockIconStop:(id)sender;
- (IBAction)setDockIconPlay:(id)sender;
- (IBAction)setDockIconPause:(id)sender;

@end
