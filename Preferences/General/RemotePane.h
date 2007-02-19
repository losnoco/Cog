//
//  FileDrawerPane.h
//  Preferences
//
//  Created by Vincent Spader on 9/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "PreferencePane.h"


@interface RemotePane : PreferencePane {
	IBOutlet NSButton *onlyOnActive;
}

- (IBAction)takeBool:(id)sender;

@end
