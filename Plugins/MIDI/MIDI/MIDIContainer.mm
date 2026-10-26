//
//  MIDIContainer.mm
//  MIDI
//
//  Created by Christopher Snowhill on 10/16/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#import "MIDIContainer.h"
#import "MIDIDecoder.h"
#import "MIDIMetadataReader.h"

#import <spessasynth_core/midi.h>
#import <spessasynth_core/file.h>

#import "Logging.h"

#import <vector>

@implementation MIDIContainer

+ (NSArray *)fileTypes {
	return [MIDIDecoder fileTypes];
}

+ (NSArray *)mimeTypes {
	return [MIDIDecoder mimeTypes];
}

+ (float)priority {
	return 1.0f;
}

+ (NSArray *)urlsForContainerURL:(NSURL *)url {
	return [self urlsForContainerURL:url trackInfo:NULL];
}

+ (NSArray *)urlsForContainerURL:(NSURL *)url trackInfo:(NSDictionary **)trackInfo {
	if([url fragment]) {
		return [NSMutableArray arrayWithObject:url];
	}

	id audioSourceClass = NSClassFromString(@"AudioSource");
	id<CogSource> source = [audioSourceClass audioSourceForURL:url];

	if(![source open:url])
		return @[];

	if(![source seekable])
		return @[];

	[source seek:0 whence:SEEK_END];
	long size = [source tell];
	[source seek:0 whence:SEEK_SET];

	NSMutableArray *tracks = [NSMutableArray array];

	try {
		std::vector<uint8_t> data;
		data.resize(size);
		if([source read:data.data() amount:size] != size)
			return @[];

		SS_File *file = ss_file_open_from_memory(data.data(), data.size(), false);
		if(!file)
			return @[];

		SS_MIDIFile *midi = ss_midi_load(file, [[url lastPathComponent] UTF8String]);
		ss_file_close(file);
		if(!midi)
			return @[];

		/* Only format-2 SMF (and multi-track XMI mapped to format 2) expose
		 * independent subsongs.  Everything else plays as a single song. */
		size_t track_count = 1;
		if(midi->format == 2)
			track_count = midi->track_count;

		if(track_count <= 1) {
			[tracks addObject:url];
		} else {
			for(size_t i = 0; i < track_count; i++) {
				[tracks addObject:[NSURL URLWithString:[[url absoluteString] stringByAppendingFormat:@"#%zu", i]]];
			}
		}

		if(trackInfo) {
			/* Every subsong's tags and length from the file just read, as
			 * the metadata reader and the decoder would find them.  The tags
			 * come from the file as loaded, the lengths from the file as the
			 * decoder prepares it, which may drop tracks.  An empty tag set
			 * is left to the loader: another plugin's metadata reader
			 * sharing the extension, as SID's does .mus, would be asked
			 * next.  So is a file the decoder would refuse. */
			NSDictionary *metadata = [MIDIMetadataReader metadataForFile:midi];
			double sampleRate;
			if([metadata count] && [MIDIDecoder prepareFile:midi] &&
			   (sampleRate = [MIDIDecoder sampleRate]) != 0) {
				NSMutableDictionary *info = [NSMutableDictionary dictionaryWithCapacity:[tracks count]];
				int i = 0;
				for(NSURL *track in tracks) {
					double framesLength, framesFade, secondsFade;
					BOOL looped;
					[MIDIDecoder getFramesLength:&framesLength framesFade:&framesFade secondsFade:&secondsFade looped:&looped forFile:midi track:i sampleRate:sampleRate];
					NSDictionary *properties = [MIDIDecoder propertiesWithSampleRate:sampleRate totalFrames:framesLength + framesFade];
					NSMutableDictionary *entryInfo = [properties mutableCopy];
					[entryInfo addEntriesFromDictionary:metadata];
					info[track] = entryInfo;
					++i;
				}
				*trackInfo = info;
			}
		}

		ss_midi_free(midi);
	} catch (std::exception &e) {
		ALog(@"Exception caught processing MIDI track count: %s", e.what());
		return @[];
	}

	return tracks;
}

@end
