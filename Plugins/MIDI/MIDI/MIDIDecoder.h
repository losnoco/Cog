//
//  MIDIDecoder.h
//  MIDI
//
//  Created by Christopher Snowhill on 10/15/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#import <Foundation/Foundation.h>

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
	double secondsFade;
	double framesFade;
	long framesRead;

	float outputBuffer[1024 * 2];
}

// The rate the decoder plays at, from the user's settings: 0 if the chosen
// synthesizer cannot run.
+ (double)sampleRate;
// Readies a freshly loaded file for playback, dropping its EMIDI tracks for
// other sound cards; NO if nothing is left to play.
+ (BOOL)prepareFile:(SS_MIDIFile *)midi;
// The length of subsong `track` of a prepared file, without and with its
// fade, as the decoder plays it at `sampleRate`.
+ (void)getFramesLength:(double *)framesLength framesFade:(double *)framesFade secondsFade:(double *)secondsFade looped:(BOOL *)looped forFile:(const SS_MIDIFile *)midi track:(int)track sampleRate:(double)sampleRate;
// The properties of a track that plays `totalFrames` frames at `sampleRate`.
+ (NSDictionary *)propertiesWithSampleRate:(double)sampleRate totalFrames:(double)totalFrames;

@end
