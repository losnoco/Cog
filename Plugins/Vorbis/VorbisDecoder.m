//
//  VorbisFile.m
//  zyVorbis
//
//  Created by Vincent Spader on 1/22/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "VorbisDecoder.h"

#import "Logging.h"

#import "HTTPSource.h"

@implementation VorbisDecoder

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

size_t sourceRead(void *buf, size_t size, size_t nmemb, void *datasource) {
	id source = (__bridge id)datasource;

	return [source read:buf amount:(size * nmemb)];
}

int sourceSeek(void *datasource, ogg_int64_t offset, int whence) {
	id source = (__bridge id)datasource;
	return ([source seek:offset whence:whence] ? 0 : -1);
}

int sourceClose(void *datasource) {
	return 0;
}

long sourceTell(void *datasource) {
	id source = (__bridge id)datasource;

	return [source tell];
}

- (BOOL)open:(id<CogSource>)s {
	source = s;

	ov_callbacks callbacks = {
		.read_func = sourceRead,
		.seek_func = sourceSeek,
		.close_func = sourceClose,
		.tell_func = sourceTell
	};

	if(ov_open_callbacks((__bridge void *)(source), &vorbisRef, NULL, 0, callbacks) != 0) {
		DLog(@"FAILED TO OPEN VORBIS FILE");
		return NO;
	}

	vorbis_info *vi;

	vi = ov_info(&vorbisRef, -1);

	bitrate = (vi->bitrate_nominal / 1000.0);
	channels = vi->channels;
	frequency = vi->rate;

	seekable = ov_seekable(&vorbisRef);

	totalFrames = ov_pcm_total(&vorbisRef, -1);

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
	const vorbis_comment *comment = ov_comment(&vorbisRef, -1);

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
}

- (void)updateIcyMetadata {
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
		vorbis_info *vi;
		vi = ov_info(&vorbisRef, -1);

		bitrate = (vi->bitrate_nominal / 1000.0);
		channels = vi->channels;
		frequency = vi->rate;

		[self willChangeValueForKey:@"properties"];
		[self didChangeValueForKey:@"properties"];

		[self updateMetadata];
	}

	do {
		lastSection = currentSection;
		float **pcm;
		numread = (int)ov_read_float(&vorbisRef, &pcm, frames - total, &currentSection);
		if(numread > 0) {
			if(channels <= MAXCHANNELS) {
				for(int i = 0; i < channels; i++) {
					for(int j = 0; j < numread; j++) {
						((float *)buf)[(total + j) * channels + i] = pcm[chmap[channels - 1][i]][j];
					}
				}
			} else {
				for(int i = 0; i < channels; i++) {
					for(int j = 0; j < numread; j++) {
						((float *)buf)[(total + j) * channels + i] = pcm[i][j];
					}
				}
			}
			total += numread;
		}

		if(currentSection != lastSection) {
			break;
		}

	} while(total != frames && numread != 0);

	[self updateIcyMetadata];

	return total;
}

- (void)close {
	ov_clear(&vorbisRef);
}

- (void)dealloc {
	[self close];
}

- (long)seek:(long)frame {
	ov_pcm_seek(&vorbisRef, frame);

	return frame;
}

- (NSDictionary *)properties {
	return @{@"channels": [NSNumber numberWithInt:channels],
			 @"bitsPerSample": [NSNumber numberWithInt:32],
			 @"floatingPoint": [NSNumber numberWithBool:YES],
			 @"sampleRate": [NSNumber numberWithFloat:frequency],
			 @"totalFrames": [NSNumber numberWithDouble:totalFrames],
			 @"bitrate": [NSNumber numberWithInt:bitrate],
			 @"seekable": [NSNumber numberWithBool:([source seekable] && seekable)],
			 @"codec": @"Ogg Vorbis",
			 @"endian": @"host",
			 @"encoding": @"lossy"};
}

- (NSDictionary *)metadata {
	return @{ @"genre": genre, @"album": album, @"artist": artist, @"title": title };
}

+ (NSArray *)fileTypes {
	return @[@"ogg"];
}

+ (NSArray *)mimeTypes {
	return @[@"application/ogg", @"application/x-ogg", @"audio/ogg", @"audio/x-vorbis+ogg"];
}

+ (float)priority {
	return 1.0;
}

+ (NSArray *)fileTypeAssociations {
	return @[
		@[@"Ogg Vorbis File", @"ogg.icns", @"ogg"]
	];
}

@end
