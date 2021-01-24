//
//  MIDIPane.h
//  General
//
//  Created by Christopher Snowhill on 10/15/13.
//
//

#import <Cocoa/Cocoa.h>
#import "GeneralPreferencePane.h"

@interface MIDIPane : GeneralPreferencePane {
    IBOutlet NSPopUpButton *midiFlavorControl;
}

- (IBAction)setSoundFont:(id)sender;

@end
