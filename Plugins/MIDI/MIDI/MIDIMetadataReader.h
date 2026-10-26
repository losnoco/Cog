//
//  MIDIMetadataReader.h
//  MIDI
//
//  Created by Christopher Snowhill on 10/16/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "Plugin.h"

#import <spessasynth_core/midi.h>

@interface MIDIMetadataReader : NSObject <CogMetadataReader> {
}

// The tags of a file just loaded, before the decoder prepares it: the same
// for every subsong.
+ (NSDictionary *)metadataForFile:(const SS_MIDIFile *)midi;

@end
