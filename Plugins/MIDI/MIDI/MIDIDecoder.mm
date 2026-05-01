//
//  MIDIDecoder.mm
//  MIDI
//
//  Created by Christopher Snowhill on 10/15/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#import "MIDIDecoder.h"

#import "AUPlayer.h"
#import "MSPlayer.h"
#import "SCPlayer.h"
#import "SpessaPlayer.h"

#import "Logging.h"

#import <spessasynth_core/file.h>
#import <spessasynth_core/midi.h>

#import "PlaylistController.h"

#import "SandboxBroker.h"

#import <dlfcn.h>

#import <cmath>
#import <vector>

static OSType getOSType(const char *in_) {
	const unsigned char *in = (const unsigned char *)in_;
	OSType v = (in[0] << 24) + (in[1] << 16) + (in[2] << 8) + in[3];
	return v;
}

@implementation MIDIDecoder

+ (NSInteger)testExtensions:(NSURL *)urlMinusExtension extensions:(NSArray *)extensionsToTest {
	NSInteger i = 0;
	for(NSString *extension in extensionsToTest) {
		id audioSourceClass = NSClassFromString(@"AudioSource");
		NSURL *url = [urlMinusExtension URLByAppendingPathExtension:extension];
		id<CogSource> src = [audioSourceClass audioSourceForURL:url];
		if([src open:url]) {
			[src close];
			return i;
		}
		++i;
	}
	return -1;
}

static double subsong_start_seconds(const SS_MIDIFile *midi, size_t subsong) {
	if(!midi || midi->format != 2 || subsong == 0) return 0.0;
	if(subsong >= midi->track_count) return 0.0;
	const SS_MIDITrack *prev = &midi->tracks[subsong - 1];
	if(prev->event_count == 0) return 0.0;
	return ss_midi_ticks_to_seconds(midi, prev->events[prev->event_count - 1].ticks);
}

static double subsong_end_seconds(const SS_MIDIFile *midi, size_t subsong) {
	if(!midi) return 0.0;
	if(midi->format != 2)
		return midi->duration;
	if(subsong >= midi->track_count) return midi->duration;
	const SS_MIDITrack *tr = &midi->tracks[subsong];
	if(tr->event_count == 0) return subsong_start_seconds(midi, subsong);
	return ss_midi_ticks_to_seconds(midi, tr->events[tr->event_count - 1].ticks);
}

- (BOOL)open:(id<CogSource>)s {
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

	double loopStart = -1;
	double loopEnd = -1;
	SS_File *file = cog_file_open_handle(s);
	if(!file)
		return NO;

	midi_file = ss_midi_load(file, [[[s url] lastPathComponent] UTF8String]);
	ss_file_close(file);
	if(!midi_file)
		return NO;

	if(ss_midi_has_emidi(midi_file)) {
		ss_midi_remove_emidi_non_gm(midi_file);
	}

	if(midi_file->duration <= 0.0) {
		ss_midi_free(midi_file);
		midi_file = NULL;
		return NO;
	}

	track_num = [[[s url] fragment] intValue]; /* 0 when absent */

	double subsong_begin = subsong_start_seconds(midi_file, (size_t)track_num);
	double subsong_end = subsong_end_seconds(midi_file, (size_t)track_num);
	framesLength = subsong_end - subsong_begin;

	if(midi_file->loop.end > 0) {
		loopStart = ss_midi_ticks_to_seconds(midi_file, midi_file->loop.start);
		loopEnd = ss_midi_ticks_to_seconds(midi_file, midi_file->loop.end);
		/* Express loop boundaries relative to subsong start. */
		loopStart -= subsong_begin;
		loopEnd -= subsong_begin;
		if(loopStart < 0.0) loopStart = 0.0;
		if(loopEnd < 0.0) loopEnd = 0.0;
	}

	if(loopStart == -1) loopStart = 0;
	if(loopEnd == -1) loopEnd = framesLength;

	if(loopStart != 0 || loopEnd != framesLength) {
		double defaultFade = [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] valueForKey:@"synthDefaultFadeSeconds"] doubleValue];
		if(defaultFade < 0.0) {
			defaultFade = 0.0;
		}
		framesLength = loopStart + (loopEnd - loopStart) * 2;
		secondsFade = defaultFade;
		isLooped = YES;
	} else {
		secondsFade = 0.0;
		isLooped = NO;
	}

	NSString *plugin = [[NSUserDefaults standardUserDefaults] stringForKey:@"midiPlugin"];
	if([plugin isEqualToString:@"NukeSc55"]) {
		sampleRate = SCPlayer::sampleRate();
		if(!sampleRate)
			return NO;
	}

	framesLength = round(framesLength * sampleRate);
	framesFade = round(secondsFade * sampleRate);

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
		NSArray *extensions = @[@"sflist", @"sf2pack", @"sf2", @"sf3", @"json", @"dls", @"sflist", @"json"];
		NSURL *fileUrl = [source url];
		NSURL *soundFontUrl = fileUrl;
		NSInteger extFound;
		if((extFound = [MIDIDecoder testExtensions:fileUrl extensions:extensions]) < 0) {
			soundFontUrl = [fileUrl URLByDeletingPathExtension];
			if((extFound = [MIDIDecoder testExtensions:soundFontUrl extensions:extensions]) < 0) {
				NSURL *urlBase = [fileUrl URLByDeletingLastPathComponent];
				NSString *dirName = [urlBase lastPathComponent];
				soundFontUrl = [urlBase URLByAppendingPathComponent:dirName];
				extFound = [MIDIDecoder testExtensions:soundFontUrl extensions:extensions];
			}
		}
		if(extFound >= 0) {
			soundFontPath = [[soundFontUrl URLByAppendingPathExtension:[extensions objectAtIndex:extFound]] absoluteString];
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

	if(![[NSFileManager defaultManager] fileExistsAtPath:globalSoundFontPath]) {
		globalSoundFontPath = nil;
		[[NSUserDefaults standardUserDefaults] setValue:globalSoundFontPath forKey:@"soundFontPath"];
	}

	NSString *plugin = [[NSUserDefaults standardUserDefaults] stringForKey:@"midiPlugin"];

	BOOL bassmidi = plugin && [plugin isEqualToString:@"BASSMIDI"];
	if(bassmidi) {
		plugin = @"Spessa";
		[[NSUserDefaults standardUserDefaults] setValue:plugin forKey:@"midiPlugin"];

		dispatch_sync(dispatch_get_main_queue(), ^{
			NSAlert *alert = [NSAlert new];
			[alert setMessageText:[[NSBundle mainBundle] localizedStringForKey:@"BassNoticeTitle" value:@"MIDI Synthesizer Notice" table:nil]];
			[alert setInformativeText:[[NSBundle mainBundle] localizedStringForKey:@"BassNoticeText" value:@"The BASSMIDI synthesizer has been replaced by SpessaSynth, an open-source SoundFont engine with support for SF2, SF3, and DLS banks. Your existing SoundFont selection has been preserved. If you experience any differences in MIDI playback, please report them via the GitHub issue tracker." table:nil]];

			[alert beginSheetModalForWindow:[NSApp mainWindow] completionHandler:^(NSModalResponse returnCode) {
			}];
		});

		[[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"midiPluginBassNoMore"];
	}

	BOOL sauce = plugin && [plugin isEqualToString:@"sauce"];
	if(sauce || !plugin || [plugin isEqualToString:@"Spessa"]) {
		if(sauce || !globalSoundFontPath || [globalSoundFontPath isEqualToString:@""]) {
			if(midi_file && ss_midi_has_gs(midi_file))
				globalSoundFontPath = [[NSBundle mainBundle] pathForResource:@"tg300b.sflist" ofType:@"json"];
			else
				globalSoundFontPath = [[NSBundle mainBundle] pathForResource:@"GeneralUserXG-SFeTest" ofType:@"sf3"];
			plugin = @"Spessa";
		}
	}

	if(midi_file && midi_file->embedded_soundbank && midi_file->embedded_soundbank_size > 0) {
		plugin = @"Spessa";
	}

	try {
		if(!plugin || [plugin isEqualToString:@"Spessa"]) {
			spessaplayer = new SpessaPlayer;

			SS_InterpolationType interp = SS_INTERP_LINEAR;
			NSString *resampling = [[NSUserDefaults standardUserDefaults] stringForKey:@"resampling"];
			if([resampling isEqualToString:@"none"] ||
			   [resampling isEqualToString:@"blep"])
				interp = SS_INTERP_NEAREST;
			else if([resampling isEqualToString:@"cubic"])
				interp = SS_INTERP_HERMITE;
			else if([resampling isEqualToString:@"sinc"])
				interp = SS_INTERP_SINC;

			spessaplayer->setSampleRate(sampleRate);
			spessaplayer->setInterpolation(interp);

			if([soundFontPath length])
				spessaplayer->setFileSoundFont([soundFontPath UTF8String]);

			player = spessaplayer;
		} else if([plugin isEqualToString:@"NukeSc55"]) {
			scplayer = new SCPlayer;

			scplayer->setUrl([source url]);
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

		unsigned int loop_mode = framesFade > 0.0 ? MIDIPlayer::loop_mode_enable | MIDIPlayer::loop_mode_force : 0;

		if(!player->Load(midi_file, (unsigned)track_num, loop_mode, secondsFade))
			return NO;
	} catch (std::exception &e) {
		ALog(@"Exception caught while loading MIDI file into player: %s", e.what());
		return NO;
	}

	NSInteger loopCount = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] integerForKey:@"synthDefaultLoopCount"];
	if(loopCount >= 2) {
		player->setLoopCount(loopCount - 1);
	}

	return YES;
}

- (AudioChunk *)readAudio {
	BOOL repeatone = IsRepeatOneSet();
	long localFramesLength = (long)framesLength;
	long localTotalFrames = (long)totalFrames;

	if(!player) {
		if(![self initDecoder])
			return nil;
	}

	double streamTimestamp = 0.0;

	try {
		player->setLoopMode(repeatone ? (MIDIPlayer::loop_mode_enable | MIDIPlayer::loop_mode_force) : 0);

		if((auplayer || spessaplayer) && !soundFontsAssigned) {
			if(globalSoundFontPath != nil) {
				if(spessaplayer)
					spessaplayer->setSoundFont([globalSoundFontPath UTF8String]);
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
		if(scplayer) {
			scplayer->flushOnSeek();
		}
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
	auplayer = NULL;
	scplayer = NULL;
	spessaplayer = NULL;

	if(midi_file) {
		ss_midi_free(midi_file);
		midi_file = NULL;
	}

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
	return @[@"mid", @"midi", @"kar", @"rmi", @"mids", @"mds", @"hmi", @"hmp", @"hmq", @"mus", @"xmi", @"lds", @"xmf", @"mxmf"];
}

+ (NSArray *)mimeTypes {
	return @[@"audio/midi", @"audio/x-midi", @"audio/xmf", @"audio/mobile-xmf", @"audio/vnd.nokia.mobile-xmf"];
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
		@[@"Loudness MIDI File", @"song.icns", @"lds"],
		@[@"Extensible Music File", @"song.icns", @"xmf"],
		@[@"Mobile Extensible Music File", @"song.icns", @"mxmf"]
	];
}

@end
