//
//  MIDIPane.h
//  General
//
//  Created by Christopher Snowhill on 10/15/13.
//
//

#import "GeneralPreferencePane.h"
#import <Cocoa/Cocoa.h>

@interface MIDIPane : GeneralPreferencePane {
	IBOutlet NSArrayController *midiPluginBehaviorArrayController;
	IBOutlet NSButton *midiPluginSetupButton;
	IBOutlet NSPopUpButton *midiPluginControl;
	IBOutlet NSPopUpButton *midiFlavorControl;
}

- (IBAction)setupPlugin:(id)sender;

- (IBAction)setSoundFont:(id)sender;

@end
