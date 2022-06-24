//
//  OutputPane.h
//  Preferences
//
//  Created by Vincent Spader on 9/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "GeneralPreferencePane.h"
#import "OutputsArrayController.h"
#import <Cocoa/Cocoa.h>

@interface OutputPane : GeneralPreferencePane {
	IBOutlet OutputsArrayController *outputDevices;
}

- (IBAction)takeDeviceID:(id)sender;

@end
