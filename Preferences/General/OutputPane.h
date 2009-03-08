//
//  OutputPane.h
//  Preferences
//
//  Created by Vincent Spader on 9/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "GeneralPreferencePane.h"
#import "OutputsArrayController.h"

@interface OutputPane : GeneralPreferencePane {
	IBOutlet OutputsArrayController *outputDevices;
}

- (IBAction) takeDeviceID:(id)sender;

@end
