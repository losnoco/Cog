//
//  PreferencesController.h
//  Preferences
//
//  Created by Vincent Spader on 9/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
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
