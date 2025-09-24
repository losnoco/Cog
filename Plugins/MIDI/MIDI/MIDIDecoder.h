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
class SCPlayer;

@interface MIDIDecoder : NSObject <CogDecoder> {
	id<CogSource> source;
	int track_num;

	BMPlayer* bmplayer;
	AUPlayer* auplayer;
	SCPlayer* scplayer;
	MIDIPlayer* player;
	midi_container midi_file;

	const void* sbHandle;

	NSString* globalSoundFontPath;
	BOOL soundFontsAssigned;
	BOOL isLooped;

	double sampleRate;

	long totalFrames;
	long framesLength;
	long framesFade;
	long framesRead;

	float outputBuffer[1024 * 2];
}

@end
