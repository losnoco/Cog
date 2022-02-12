//
//  OpusDecoder.m
//  Opus
//
//  Created by Christopher Snowhill on 10/4/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#import "Plugin.h"

#import "OpusDecoder.h"

#import "Logging.h"

#import "HTTPSource.h"

@implementation OpusFile

static const int MAXCHANNELS = 8;
static const int chmap[MAXCHANNELS][MAXCHANNELS] = {
	{
	0,
	}, // mono
	{
	0,
	1,
	}, // l, r
	{
	0,
	2,
	1,
	}, // l, c, r -> l, r, c
	{
	0,
	1,
	2,
	3,
	}, // l, r, bl, br
	{
	0,
	2,
	1,
	3,
	4,
	}, // l, c, r, bl, br -> l, r, c, bl, br
	{ 0, 2, 1, 5, 3, 4 }, // l, c, r, bl, br, lfe -> l, r, c, lfe, bl, br
	{ 0, 2, 1, 6, 5, 3, 4 }, // l, c, r, sl, sr, bc, lfe -> l, r, c, lfe, bc, sl, sr
	{ 0, 2, 1, 7, 5, 6, 3, 4 } // l, c, r, sl, sr, bl, br, lfe -> l, r, c, lfe, bl, br, sl, sr
};

int sourceRead(void *_stream, unsigned char *_ptr, int _nbytes) {
	id source = (__bridge id)_stream;

	return (int)[source read:_ptr amount:_nbytes];
}

int sourceSeek(void *_stream, opus_int64 _offset, int _whence) {
	id source = (__bridge id)_stream;
	return ([source seek:_offset whence:_whence] ? 0 : -1);
}

int sourceClose(void *_stream) {
	return 0;
}

opus_int64 sourceTell(void *_stream) {
	id source = (__bridge id)_stream;

	return [source tell];
}

- (id)init {
	self = [super init];
	if(self) {
		opusRef = NULL;
	}
	return self;
}

- (BOOL)open:(id<CogSource>)s {
	source = s;

	OpusFileCallbacks callbacks = {
		.read = sourceRead,
		.seek = sourceSeek,
		.close = sourceClose,
		.tell = sourceTell
	};

	int error;
	opusRef = op_open_callbacks((__bridge void *)source, &callbacks, NULL, 0, &error);

	if(!opusRef) {
		DLog(@"FAILED TO OPEN OPUS FILE");
		return NO;
	}

	currentSection = lastSection = op_current_link(opusRef);

	bitrate = (op_bitrate(opusRef, currentSection) / 1000.0);
	channels = op_channel_count(opusRef, currentSection);

	seekable = op_seekable(opusRef);

	totalFrames = op_pcm_total(opusRef, -1);

	const OpusHead *head = op_head(opusRef, -1);
	const OpusTags *tags = op_tags(opusRef, -1);

	int _track_gain = 0;

	opus_tags_get_track_gain(tags, &_track_gain);

	replayGainAlbumGain = ((double)head->output_gain / 256.0) + 5.0;
	replayGainTrackGain = ((double)_track_gain / 256.0) + replayGainAlbumGain;

	op_set_gain_offset(opusRef, OP_ABSOLUTE_GAIN, 0);

	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];

	artist = @"";
	albumartist = @"";
	album = @"";
	title = @"";
	genre = @"";
	year = @(0);
	track = @(0);
	disc = @(0);
	albumArt = [NSData data];
	[self updateMetadata];

	return YES;
}

- (NSString *)parseTag:(NSString *)tag fromTags:(const OpusTags *)tags {
	NSMutableArray *tagStrings = [[NSMutableArray alloc] init];

	int tagCount = opus_tags_query_count(tags, [tag UTF8String]);

	for(int i = 0; i < tagCount; ++i) {
		const char *value = opus_tags_query(tags, [tag UTF8String], i);
		[tagStrings addObject:[NSString stringWithUTF8String:value]];
	}

	return [tagStrings componentsJoinedByString:@", "];
}

- (void)updateMetadata {
	const OpusTags *tags = op_tags(opusRef, -1);

	if(tags) {
		NSString *_artist = [self parseTag:@"artist" fromTags:tags];
		NSString *_albumartist = [self parseTag:@"albumartist" fromTags:tags];
		NSString *_album = [self parseTag:@"album" fromTags:tags];
		NSString *_title = [self parseTag:@"title" fromTags:tags];
		NSString *_genre = [self parseTag:@"genre" fromTags:tags];

		NSString *_yearDate = [self parseTag:@"date" fromTags:tags];
		NSString *_yearYear = [self parseTag:@"year" fromTags:tags];

		NSNumber *_year = @(0);
		if([_yearDate length])
			_year = @([_yearDate intValue]);
		else if([_yearYear length])
			_year = @([_yearYear intValue]);

		NSString *_trackNumber = [self parseTag:@"tracknumber" fromTags:tags];
		NSString *_trackNum = [self parseTag:@"tracknum" fromTags:tags];
		NSString *_trackTrack = [self parseTag:@"track" fromTags:tags];

		NSNumber *_track = @(0);
		if([_trackNumber length])
			_track = @([_trackNumber intValue]);
		else if([_trackNum length])
			_track = @([_trackNum intValue]);
		else if([_trackTrack length])
			_track = @([_trackTrack intValue]);

		NSString *_discNumber = [self parseTag:@"discnumber" fromTags:tags];
		NSString *_discNum = [self parseTag:@"discnum" fromTags:tags];
		NSString *_discDisc = [self parseTag:@"disc" fromTags:tags];

		NSNumber *_disc = @(0);
		if([_discNumber length])
			_disc = @([_discNumber intValue]);
		else if([_discNum length])
			_disc = @([_discNum intValue]);
		else if([_discDisc length])
			_disc = @([_discDisc intValue]);

		NSData *_albumArt = [NSData data];

		size_t count = opus_tags_query_count(tags, "METADATA_BLOCK_PICTURE");
		if(count) {
			const char *pictureTag = opus_tags_query(tags, "METADATA_BLOCK_PICTURE", 0);
			OpusPictureTag _pic = { 0 };
			if(opus_picture_tag_parse(&_pic, pictureTag) >= 0) {
				if(_pic.format == OP_PIC_FORMAT_PNG ||
				   _pic.format == OP_PIC_FORMAT_JPEG ||
				   _pic.format == OP_PIC_FORMAT_GIF) {
					_albumArt = [NSData dataWithBytes:_pic.data length:_pic.data_length];
				}
				opus_picture_tag_clear(&_pic);
			}
		}

		if(![_artist isEqual:artist] ||
		   ![_albumartist isEqual:albumartist] ||
		   ![_album isEqual:album] ||
		   ![_title isEqual:title] ||
		   ![_genre isEqual:genre] ||
		   ![_year isEqual:year] ||
		   ![_track isEqual:year] ||
		   ![_disc isEqual:disc] ||
		   ![_albumArt isEqual:albumArt]) {
			artist = _artist;
			albumartist = _albumartist;
			album = _album;
			title = _title;
			genre = _genre;
			year = _year;
			track = _track;
			disc = _disc;
			albumArt = _albumArt;

			[self willChangeValueForKey:@"metadata"];
			[self didChangeValueForKey:@"metadata"];
		}
	}
}

- (void)updateIcyMetadata {
	if([source seekable]) return;

	NSString *_genre = genre;
	NSString *_album = album;
	NSString *_artist = artist;
	NSString *_title = title;

	Class sourceClass = [source class];
	if([sourceClass isEqual:NSClassFromString(@"HTTPSource")]) {
		HTTPSource *httpSource = (HTTPSource *)source;
		if([httpSource hasMetadata]) {
			NSDictionary *metadata = [httpSource metadata];
			_genre = [metadata valueForKey:@"genre"];
			_album = [metadata valueForKey:@"album"];
			_artist = [metadata valueForKey:@"artist"];
			_title = [metadata valueForKey:@"title"];
		}
	}

	if(![_genre isEqual:genre] ||
	   ![_album isEqual:album] ||
	   ![_artist isEqual:artist] ||
	   ![_title isEqual:title]) {
		genre = _genre;
		album = _album;
		artist = _artist;
		title = _title;
		[self willChangeValueForKey:@"metadata"];
		[self didChangeValueForKey:@"metadata"];
	}
}

- (int)readAudio:(void *)buf frames:(UInt32)frames {
	int numread;
	int total = 0;

	if(currentSection != lastSection) {
		bitrate = (op_bitrate(opusRef, currentSection) / 1000.0);
		channels = op_channel_count(opusRef, currentSection);

		[self willChangeValueForKey:@"properties"];
		[self didChangeValueForKey:@"properties"];

		[self updateMetadata];
	}

	int size = frames * channels;

	do {
		float *out = ((float *)buf) + total;
		float tempbuf[512 * channels];
		lastSection = currentSection;
		int toread = size - total;
		if(toread > 512) toread = 512;
		numread = op_read_float(opusRef, (channels < MAXCHANNELS) ? tempbuf : out, toread, NULL);
		if(numread > 0 && channels <= MAXCHANNELS) {
			for(int i = 0; i < numread; ++i) {
				for(int j = 0; j < channels; ++j) {
					out[i * channels + j] = tempbuf[i * channels + chmap[channels - 1][j]];
				}
			}
		}
		currentSection = op_current_link(opusRef);
		if(numread > 0) {
			total += numread * channels;
		}

		if(currentSection != lastSection) {
			break;
		}

	} while(total != size && numread != 0);

	[self updateIcyMetadata];

	return total / channels;
}

- (void)close {
	op_free(opusRef);
	opusRef = NULL;
}

- (void)dealloc {
	[self close];
}

- (long)seek:(long)frame {
	op_pcm_seek(opusRef, frame);

	return frame;
}

- (NSDictionary *)properties {
	return @{ @"channels": [NSNumber numberWithInt:channels],
		      @"bitsPerSample": [NSNumber numberWithInt:32],
		      @"floatingPoint": [NSNumber numberWithBool:YES],
		      @"sampleRate": [NSNumber numberWithFloat:48000],
		      @"totalFrames": [NSNumber numberWithDouble:totalFrames],
		      @"bitrate": [NSNumber numberWithInt:bitrate],
		      @"seekable": [NSNumber numberWithBool:([source seekable] && seekable)],
		      @"replayGainAlbumGain": @(replayGainAlbumGain),
		      @"replayGainTrackGain": @(replayGainTrackGain),
		      @"codec": @"Opus",
		      @"endian": @"host",
		      @"encoding": @"lossy" };
}

- (NSDictionary *)metadata {
	return @{ @"artist": artist, @"albumartist": albumartist, @"album": album, @"title": title, @"genre": genre, @"year": year, @"track": track, @"disc": disc, @"albumArt": albumArt };
}

+ (NSArray *)fileTypes {
	return @[@"opus", @"ogg"];
}

+ (NSArray *)mimeTypes {
	return @[@"audio/x-opus+ogg", @"application/ogg"];
}

+ (float)priority {
	return 1.0;
}

+ (NSArray *)fileTypeAssociations {
	return @[
		@[@"Opus Audio File", @"ogg.icns", @"opus"],
		@[@"Ogg Audio File", @"ogg.icns", @"ogg"]
	];
}

@end
