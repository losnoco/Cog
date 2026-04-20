//
//  MIDIContainer.mm
//  MIDI
//
//  Created by Christopher Snowhill on 10/16/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#import "MIDIContainer.h"
#import "MIDIDecoder.h"

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

	size_t track_count = 1;

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
		if(midi->format == 2)
			track_count = midi->track_count;

		ss_midi_free(midi);
	} catch (std::exception &e) {
		ALog(@"Exception caught processing MIDI track count: %s", e.what());
		return @[];
	}

	if(track_count <= 1)
		return @[url];

	NSMutableArray *tracks = [NSMutableArray array];
	for(size_t i = 0; i < track_count; i++) {
		[tracks addObject:[NSURL URLWithString:[[url absoluteString] stringByAppendingFormat:@"#%zu", i]]];
	}
	return tracks;
}

@end
