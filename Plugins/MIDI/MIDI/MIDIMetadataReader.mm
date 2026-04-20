//
//  MIDIMetadataReader.mm
//  MIDI
//
//  Created by Christopher Snowhill on 10/16/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#import "MIDIMetadataReader.h"

#import "MIDIDecoder.h"

#import <spessasynth_core/midi.h>
#import <spessasynth_core/file.h>

#import "Logging.h"

#import <vector>

@implementation MIDIMetadataReader

+ (NSArray *)fileTypes {
	return [MIDIDecoder fileTypes];
}

+ (NSArray *)mimeTypes {
	return [MIDIDecoder mimeTypes];
}

+ (float)priority {
	return 1.0f;
}

static void setDictionary(NSMutableDictionary *dict, NSString *tag, NSString *value) {
	NSString *realKey = [tag stringByReplacingOccurrencesOfString:@"." withString:@"․"];
	NSMutableArray *array = [dict valueForKey:realKey];
	if(!array) {
		array = [NSMutableArray new];
		[dict setObject:array forKey:realKey];
	}
	[array addObject:value];
}

static NSString *nsStringFromBytes(const uint8_t *data, size_t len) {
	if(!data || !len) return nil;
	std::vector<char> buf;
	buf.resize(len + 1);
	memcpy(buf.data(), data, len);
	buf[len] = 0;
	return guess_encoding_of_string(buf.data());
}

static void addRmidField(NSMutableDictionary *dict, NSString *tag,
                         const uint8_t *data, size_t len) {
	NSString *value = nsStringFromBytes(data, len);
	if(value && [value length])
		setDictionary(dict, tag, value);
}

+ (NSDictionary *)metadataForURL:(NSURL *)url {
	id audioSourceClass = NSClassFromString(@"AudioSource");
	id<CogSource> source = [audioSourceClass audioSourceForURL:url];

	if(![source open:url])
		return 0;

	if(![source seekable])
		return 0;

	[source seek:0 whence:SEEK_END];
	long size = [source tell];
	[source seek:0 whence:SEEK_SET];

	try {
		std::vector<uint8_t> data;
		data.resize(size);
		if([source read:data.data() amount:size] != size)
			return @{};

		SS_File *file = ss_file_open_from_memory(data.data(), data.size(), false);
		if(!file) return @{};

		SS_MIDIFile *midi = ss_midi_load(file, [[url lastPathComponent] UTF8String]);
		ss_file_close(file);
		if(!midi) return @{};

		int track_num = 0;
		if([[url fragment] length])
			track_num = [[url fragment] intValue];

		NSMutableDictionary *dict = [NSMutableDictionary new];

		const SS_RMIDIInfo *ri = &midi->rmidi_info;

		addRmidField(dict, @"title", ri->name, ri->name_len);
		addRmidField(dict, @"artist", ri->artist, ri->artist_len);
		addRmidField(dict, @"album", ri->album, ri->album_len);
		addRmidField(dict, @"genre", ri->genre, ri->genre_len);
		addRmidField(dict, @"comment", ri->comment, ri->comment_len);
		addRmidField(dict, @"copyright", ri->copyright, ri->copyright_len);
		addRmidField(dict, @"date", ri->creation_date, ri->creation_date_len);
		addRmidField(dict, @"engineer", ri->engineer, ri->engineer_len);
		addRmidField(dict, @"software", ri->software, ri->software_len);
		addRmidField(dict, @"subject", ri->subject, ri->subject_len);

		/* Per-track names and text/marker meta events */
		for(size_t ti = 0; ti < midi->track_count; ++ti) {
			const SS_MIDITrack *tr = &midi->tracks[ti];
			if(tr->name[0]) {
				NSString *name = guess_encoding_of_string(tr->name);
				if(name && [name length]) {
					NSString *key = [NSString stringWithFormat:@"track_name_%02zu", ti];
					setDictionary(dict, key, name);
				}
			}
			for(size_t ei = 0; ei < tr->event_count; ++ei) {
				const SS_MIDIMessage *e = &tr->events[ei];
				if(e->status_byte == 0x01 && e->data && e->data_length) {
					/* Text meta */
					NSString *key = [NSString stringWithFormat:@"track_text_%02zu", ti];
					NSString *value = nsStringFromBytes(e->data, e->data_length);
					if(value && [value length])
						setDictionary(dict, key, value);
				} else if(e->status_byte == 0x06 && e->data && e->data_length) {
					/* Marker meta */
					NSString *value = nsStringFromBytes(e->data, e->data_length);
					if(value && [value length])
						setDictionary(dict, @"track_marker", value);
				}
			}
		}

		/* Fall back to filename-based title when no title was produced. */
		if(![dict valueForKey:@"title"]) {
			NSString *fallback = nil;
			if(midi->track_count > 0 && midi->tracks[0].name[0])
				fallback = guess_encoding_of_string(midi->tracks[0].name);
			if(fallback && [fallback length])
				setDictionary(dict, @"title", fallback);
		}

		if(ri->picture && ri->picture_len) {
			@autoreleasepool {
				[dict setObject:[NSData dataWithBytes:ri->picture length:ri->picture_len] forKey:@"albumArt"];
			}
		}

		(void)track_num;

		ss_midi_free(midi);
		return dict;
	} catch (std::exception &e) {
		ALog(@"Exception caught while reading MIDI metadata: %s", e.what());
		return @{};
	}
}

@end
