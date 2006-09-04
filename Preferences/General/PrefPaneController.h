//
//  PreferencesController.h
//  Preferences
//
//  Created by Zaphod Beeblebrox on 9/4/06.
//  Copyright 2006 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "SS_PreferencePaneProtocol.h"

#import "HotKeyPane.h"
#import "FileDrawerPane.h"

@interface PrefPaneController : NSObject <SS_PreferencePaneProtocol> {
	IBOutlet HotKeyPane *hotKeyPane;
	IBOutlet FileDrawerPane *fileDrawerPane;
}

- (FileDrawerPane *)fileDrawerPane;
- (HotKeyPane *)hotKeyPane;

@end
