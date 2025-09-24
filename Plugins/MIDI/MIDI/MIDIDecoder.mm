//
//  MIDIDecoder.mm
//  MIDI
//
//  Created by Christopher Snowhill on 10/15/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#import "MIDIDecoder.h"

#import "AUPlayer.h"
#import "BMPlayer.h"
#import "MSPlayer.h"
#import "SCPlayer.h"

#import "Logging.h"

#import <midi_processing/midi_processor.h>

#import "PlaylistController.h"

#import "SandboxBroker.h"

#import <dlfcn.h>

static OSType getOSType(const char *in_) {
	const unsigned char *in = (const unsigned char *)in_;
	OSType v = (in[0] << 24) + (in[1] << 16) + (in[2] << 8) + in[3];
	return v;
}

@implementation MIDIDecoder

+ (NSInteger)testExtensions:(NSString *)pathMinusExtension extensions:(NSArray *)extensionsToTest {
	NSInteger i = 0;
	for(NSString *extension in extensionsToTest) {
		if([[NSFileManager defaultManager] fileExistsAtPath:[pathMinusExtension stringByAppendingPathExtension:extension]])
			return i;
		++i;
	}
	return -1;
}

- (BOOL)open:(id<CogSource>)s {
	// We need file-size to use midi_processing
	if(![s seekable]) {
		return NO;
	}

	sampleRate = [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] valueForKey:@"synthSampleRate"] doubleValue];
	if(sampleRate < 8000.0) {
		sampleRate = 44100.0;
	} else if(sampleRate > 192000.0) {
		sampleRate = 192000.0;
	}

	source = s;

	unsigned long loopStart = ~0;
	unsigned long loopEnd = ~0;

	try {
		std::vector<uint8_t> file_data;

		[s seek:0 whence:SEEK_END];
		size_t size = [s tell];
		[s seek:0 whence:SEEK_SET];
		file_data.resize(size);
		[s read:&file_data[0] amount:size];

		if(!midi_processor::process_file(file_data, [[[s url] pathExtension] UTF8String], midi_file))
			return NO;

		if(!midi_file.get_timestamp_end(track_num))
			return NO;

		track_num = [[[s url] fragment] intValue]; // What if theres no fragment? Assuming we get 0.

		midi_file.scan_for_loops(true, true, true, true);

		framesLength = midi_file.get_timestamp_end(track_num, true);

		loopStart = midi_file.get_timestamp_loop_start(track_num, true);
		loopEnd = midi_file.get_timestamp_loop_end(track_num, true);
	} catch (std::exception &e) {
		ALog(@"Exception caught while reading MIDI file: %s", e.what());
		return NO;
	}

	if(loopStart == ~0UL) loopStart = 0;
	if(loopEnd == ~0UL) loopEnd = framesLength;

	if(loopStart != 0 || loopEnd != framesLength) {
		// two loops and a fade
		double defaultFade = [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] valueForKey:@"synthDefaultFadeSeconds"] doubleValue];
		if(defaultFade < 0.0) {
			defaultFade = 0.0;
		}
		framesLength = loopStart + (loopEnd - loopStart) * 2;
		framesFade = (int)ceil(defaultFade * 1000.0);
		isLooped = YES;
	} else {
		framesLength += 1000;
		framesFade = 0;
		isLooped = NO;
	}

	// This plugin overrides the sample rate, and this requires loading the ROM set
	NSString *plugin = [[NSUserDefaults standardUserDefaults] stringForKey:@"midiPlugin"];
	if([plugin isEqualToString:@"NukeSc55"]) {
		sampleRate = SCPlayer::sampleRate();
		if(!sampleRate)
			return NO;
	}

	framesLength = (int)ceil(framesLength * sampleRate * 0.001);
	framesFade = (int)ceil(framesFade * sampleRate * 0.001);

	totalFrames = framesLength + framesFade;

	framesRead = 0;

	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];

	return YES;
}

- (NSDictionary *)properties {
	return @{ @"bitrate": @(0),
		      @"sampleRate": @(sampleRate),
		      @"totalFrames": @(totalFrames),
		      @"bitsPerSample": @(32),
		      @"floatingPoint": @(YES),
		      @"channels": @(2),
		      @"seekable": @(YES),
		      @"codec": @"MIDI",
		      @"endian": @"host",
		      @"encoding": @"synthesized" };
}

- (NSDictionary *)metadata {
	return @{};
}

- (BOOL)initDecoder {
	NSString *soundFontPath = @"";

	if([[source url] isFileURL]) {
		// Let's check for a SoundFont
		NSArray *extensions = @[@"sflist", @"sf2pack", @"sf2", @"sf3"];
		NSString *filePath = [[source url] path];
		NSString *fileNameBase = [filePath lastPathComponent];
		filePath = [filePath stringByDeletingLastPathComponent];
		soundFontPath = [filePath stringByAppendingPathComponent:fileNameBase];
		NSInteger extFound;
		if((extFound = [MIDIDecoder testExtensions:soundFontPath extensions:extensions]) < 0) {
			fileNameBase = [fileNameBase stringByDeletingPathExtension];
			soundFontPath = [filePath stringByAppendingPathComponent:fileNameBase];
			if((extFound = [MIDIDecoder testExtensions:soundFontPath extensions:extensions]) < 0) {
				fileNameBase = [filePath lastPathComponent];
				soundFontPath = [filePath stringByAppendingPathComponent:fileNameBase];
				extFound = [MIDIDecoder testExtensions:soundFontPath extensions:extensions];
			}
		}
		if(extFound >= 0) {
			soundFontPath = [soundFontPath stringByAppendingPathExtension:[extensions objectAtIndex:extFound]];
		} else
			soundFontPath = @"";
	}

	DLog(@"Length: %li", totalFrames);

	DLog(@"Track num: %i", track_num);

	MIDIPlayer::filter_mode mode = MIDIPlayer::filter_sc55;

	NSString *flavor = [[NSUserDefaults standardUserDefaults] stringForKey:@"midi.flavor"];
	if([flavor isEqualToString:@"default"])
		mode = MIDIPlayer::filter_default;
	else if([flavor isEqualToString:@"gm"])
		mode = MIDIPlayer::filter_gm;
	else if([flavor isEqualToString:@"gm2"])
		mode = MIDIPlayer::filter_gm2;
	else if([flavor isEqualToString:@"sc55"])
		mode = MIDIPlayer::filter_sc55;
	else if([flavor isEqualToString:@"sc88"])
		mode = MIDIPlayer::filter_sc88;
	else if([flavor isEqualToString:@"sc88pro"])
		mode = MIDIPlayer::filter_sc88pro;
	else if([flavor isEqualToString:@"sc8850"])
		mode = MIDIPlayer::filter_sc8850;
	else if([flavor isEqualToString:@"xg"])
		mode = MIDIPlayer::filter_xg;

	globalSoundFontPath = [[NSUserDefaults standardUserDefaults] stringForKey:@"soundFontPath"];

	if(globalSoundFontPath && [globalSoundFontPath length] > 0) {
		NSURL *sandboxURL = [NSURL fileURLWithPath:[globalSoundFontPath stringByDeletingLastPathComponent]];

		id sandboxBrokerClass = NSClassFromString(@"SandboxBroker");
		id sandboxBroker = [sandboxBrokerClass sharedSandboxBroker];

		sbHandle = [sandboxBroker beginFolderAccess:sandboxURL];
	} else {
		sbHandle = NULL;
	}

	// First detect if soundfont has gone AWOL
	if(![[NSFileManager defaultManager] fileExistsAtPath:globalSoundFontPath]) {
		globalSoundFontPath = nil;
		[[NSUserDefaults standardUserDefaults] setValue:globalSoundFontPath forKey:@"soundFontPath"];
	}

	NSString *plugin = [[NSUserDefaults standardUserDefaults] stringForKey:@"midiPlugin"];

	// Then detect if we should force the DLSMusicSynth, which has its own bank
	BOOL sauce = plugin && [plugin isEqualToString:@"sauce"];
	if(sauce || !plugin || [plugin isEqualToString:@"BASSMIDI"]) {
		if(sauce || !globalSoundFontPath || [globalSoundFontPath isEqualToString:@""]) {
			plugin = @"dls appl"; // Apple DLSMusicSynth if soundfont doesn't exist
		}
	}

	try {
		if(!plugin || [plugin isEqualToString:@"BASSMIDI"]) {
			bmplayer = new BMPlayer;

			bool resamplingSinc = false;
			NSString *resampling = [[NSUserDefaults standardUserDefaults] stringForKey:@"resampling"];
			if([resampling isEqualToString:@"sinc"])
				resamplingSinc = true;

			bmplayer->setSincInterpolation(resamplingSinc);
			bmplayer->setSampleRate(sampleRate);

			if([soundFontPath length])
				bmplayer->setFileSoundFont([soundFontPath UTF8String]);

			player = bmplayer;
		} else if([plugin isEqualToString:@"NukeSc55"]) {
			scplayer = new SCPlayer;

			scplayer->setSampleRate(sampleRate);

			player = scplayer;
		} else if([[plugin substringToIndex:4] isEqualToString:@"DOOM"]) {
			MSPlayer *msplayer = new MSPlayer;
			player = msplayer;

			msplayer->set_synth(0);

			msplayer->set_bank([[plugin substringFromIndex:4] intValue]);

			msplayer->set_extp(1);

			msplayer->setSampleRate(sampleRate);
		} else if([[plugin substringToIndex:5] isEqualToString:@"OPL3W"]) {
			MSPlayer *msplayer = new MSPlayer;
			player = msplayer;

			msplayer->set_synth(1);

			msplayer->set_bank([[plugin substringFromIndex:5] intValue]);

			msplayer->set_extp(1);

			msplayer->setSampleRate(sampleRate);
		} else {
			const char *cplugin = [plugin UTF8String];
			OSType componentSubType;
			OSType componentManufacturer;

			componentSubType = getOSType(cplugin);
			componentManufacturer = getOSType(cplugin + 4);

			{
				auplayer = new AUPlayer;

				auplayer->setComponent(componentSubType, componentManufacturer);
				auplayer->setSampleRate(sampleRate);

				if([soundFontPath length]) {
					auplayer->setSoundFont([soundFontPath UTF8String]);
					soundFontsAssigned = YES;
				}

				NSDictionary *midiPluginSettings = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"midiPluginSettings"];
				if(midiPluginSettings) {
					NSDictionary *theSettings = [midiPluginSettings objectForKey:plugin];
					if(theSettings) {
						auplayer->setPreset(theSettings);
					}
				}

				player = auplayer;
			}
		}

		player->setFilterMode(mode, false);

		unsigned int loop_mode = framesFade ? MIDIPlayer::loop_mode_enable | MIDIPlayer::loop_mode_force : 0;
		unsigned int clean_flags = midi_container::clean_flag_emidi;

		if(!player->Load(midi_file, track_num, loop_mode, clean_flags))
			return NO;
	} catch (std::exception &e) {
		ALog(@"Exception caught while loading MIDI file into player: %s", e.what());
		return NO;
	}

	return YES;
}

- (AudioChunk *)readAudio {
	BOOL repeatone = IsRepeatOneSet();
	long localFramesLength = framesLength;
	long localTotalFrames = totalFrames;

	if(!player) {
		if(![self initDecoder])
			return nil;
	}
	
	double streamTimestamp = 0.0;

	try {
		player->setLoopMode((repeatone || isLooped) ? (MIDIPlayer::loop_mode_enable | MIDIPlayer::loop_mode_force) : 0);

		if(!repeatone && framesRead >= localTotalFrames)
			return 0;

		if((bmplayer || auplayer) && !soundFontsAssigned) {
			if(globalSoundFontPath != nil) {
				if(bmplayer)
					bmplayer->setSoundFont([globalSoundFontPath UTF8String]);
				else if(auplayer)
					auplayer->setSoundFont([globalSoundFontPath UTF8String]);
			}

			soundFontsAssigned = YES;
		}

		streamTimestamp = (double)(player->Tell()) / sampleRate;

		int frames = 1024;

		UInt32 frames_done = player->Play(outputBuffer, frames);

		if(!frames_done)
			return 0;

		frames = frames_done;

		if(!repeatone && framesRead + frames > localFramesLength) {
			if(framesFade) {
				long fadeStart = (localFramesLength > framesRead) ? localFramesLength : framesRead;
				long fadeEnd = (framesRead + frames > localTotalFrames) ? localTotalFrames : (framesRead + frames);
				long fadePos;

				float *buff = outputBuffer;

				float fadeScale = (float)(framesFade - (fadeStart - localFramesLength)) / framesFade;
				float fadeStep = 1.0 / (float)framesFade;
				for(fadePos = fadeStart; fadePos < fadeEnd; ++fadePos) {
					buff[0] *= fadeScale;
					buff[1] *= fadeScale;
					buff += 2;
					fadeScale -= fadeStep;
					if(fadeScale < 0) {
						fadeScale = 0;
						fadeStep = 0;
					}
				}

				frames = (int)(fadeEnd - framesRead);
			} else {
				frames = (int)(localTotalFrames - framesRead);
			}
		}

		framesRead += frames;

		id audioChunkClass = NSClassFromString(@"AudioChunk");
		AudioChunk *chunk = [[audioChunkClass alloc] initWithProperties:[self properties]];
		[chunk setStreamTimestamp:streamTimestamp];
		[chunk assignSamples:outputBuffer frameCount:frames];

		return chunk;
	} catch (std::exception &e) {
		ALog(@"Exception caught while playing MIDI file: %s", e.what());
		return nil;
	}
}

- (long)seek:(long)frame {
	if(!player) {
		if(![self readAudio])
			return -1;
	}

	try {
		player->Seek(frame);
	} catch (std::exception &e) {
		ALog(@"Exception caught while seeking in MIDI file: %s", e.what());
		framesRead = 0;
		return -1;
	}

	framesRead = frame;

	return frame;
}

- (void)close {
	delete player;
	player = NULL;

	if(sbHandle) {
		id sandboxBrokerClass = NSClassFromString(@"SandboxBroker");
		id sandboxBroker = [sandboxBrokerClass sharedSandboxBroker];

		[sandboxBroker endFolderAccess:sbHandle];
		sbHandle = NULL;
	}
}

- (void)dealloc {
	[self close];
}

+ (NSArray *)fileTypes {
	return @[@"mid", @"midi", @"kar", @"rmi", @"mids", @"mds", @"hmi", @"hmp", @"hmq", @"mus", @"xmi", @"lds"];
}

+ (NSArray *)mimeTypes {
	return @[@"audio/midi", @"audio/x-midi"];
}

+ (float)priority {
	return 1.0;
}

+ (NSArray *)fileTypeAssociations {
	return @[
		@[@"General MIDI File", @"song.icns", @"mid", @"midi", @"kar"],
		@[@"RIFF MIDI File", @"song.icns", @"rmi"],
		@[@"MIDS MIDI File", @"song.icns", @"mids", @"mds"],
		@[@"HMI MIDI File", @"song.icns", @"hmi", @"hmp", @"hmq"],
		@[@"id Software MUS MIDI File", @"song.icns", @"mus"],
		@[@"XMI MIDI File", @"song.icns", @"xmi"],
		@[@"Loudness MIDI File", @"song.icns", @"lds"]
	];
}

@end
