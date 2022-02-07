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
	IBOutlet NSPopUpButton *midiFlavorControl;
}

- (IBAction)setSoundFont:(id)sender;

@end
