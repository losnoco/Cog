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

	album_gain = ((double)head->output_gain / 256.0) + 5.0;
	track_gain = ((double)_track_gain / 256.0) + album_gain;

	op_set_gain_offset(opusRef, OP_ABSOLUTE_GAIN, 0);

	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];

	genre = @"";
	album = @"";
	artist = @"";
	title = @"";
	[self updateMetadata];

	return YES;
}

- (void)updateMetadata {
	const OpusTags *comment = op_tags(opusRef, -1);

	if(comment) {
		uint8_t nullByte = '\0';
		NSString *_genre = genre;
		NSString *_album = album;
		NSString *_artist = artist;
		NSString *_title = title;
		for(int i = 0; i < comment->comments; ++i) {
			NSMutableData *commentField = [NSMutableData dataWithBytes:comment->user_comments[i] length:comment->comment_lengths[i]];
			[commentField appendBytes:&nullByte length:1];
			NSString *commentString = [NSString stringWithUTF8String:[commentField bytes]];
			NSArray *splitFields = [commentString componentsSeparatedByString:@"="];
			if([splitFields count] == 2) {
				NSString *name = [splitFields objectAtIndex:0];
				NSString *value = [splitFields objectAtIndex:1];
				name = [name stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
				value = [value stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
				name = [name lowercaseString];
				if([name isEqualToString:@"genre"]) {
					_genre = value;
				} else if([name isEqualToString:@"album"]) {
					_album = value;
				} else if([name isEqualToString:@"artist"]) {
					_artist = value;
				} else if([name isEqualToString:@"title"]) {
					_title = value;
				}
			}
		}

		if(![source seekable] &&
		   (![_genre isEqual:genre] ||
		    ![_album isEqual:album] ||
		    ![_artist isEqual:artist] ||
		    ![_title isEqual:title])) {
			genre = _genre;
			album = _album;
			artist = _artist;
			title = _title;
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
		      @"replayGainTrackGain": [NSNumber numberWithFloat:track_gain],
		      @"replayGainAlbumGain": [NSNumber numberWithFloat:album_gain],
		      @"codec": @"Opus",
		      @"endian": @"host",
		      @"encoding": @"lossy" };
}

- (NSDictionary *)metadata {
	return @{ @"genre": genre, @"album": album, @"artist": artist, @"title": title };
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
