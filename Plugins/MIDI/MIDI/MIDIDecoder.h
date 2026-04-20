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
class SCPlayer;
class SpessaPlayer;

@interface MIDIDecoder : NSObject <CogDecoder> {
	id<CogSource> source;
	int track_num;

	AUPlayer *auplayer;
	SCPlayer *scplayer;
	SpessaPlayer *spessaplayer;
	MIDIPlayer *player;
	SS_MIDIFile *midi_file;

	const void *sbHandle;

	NSString *globalSoundFontPath;
	BOOL soundFontsAssigned;
	BOOL isLooped;

	double sampleRate;

	double totalFrames;
	double framesLength;
	double framesFade;
	long framesRead;

	float outputBuffer[1024 * 2];
}

@end
