//
//  MIDIDecoder.h
//  MIDI
//
//  Created by Christopher Snowhill on 10/15/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "MIDIPlayer.h"

#import "Plugin.h"

class AUPlayer;
class BMPlayer;

@interface MIDIDecoder : NSObject <CogDecoder> {
    id<CogSource> source;
    int track_num;

    BMPlayer* bmplayer;
    AUPlayer* auplayer;
	MIDIPlayer* player;
    midi_container midi_file;
    
    BOOL soundFontsAssigned;
    BOOL isLooped;
    
    long totalFrames;
    long framesLength;
    long framesFade;
    long framesRead;
}

@end
