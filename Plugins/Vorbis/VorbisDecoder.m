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

#import "picture.h"

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

- (NSString *)parseTag:(NSString *)tag fromTags:(vorbis_comment *)tags {
	NSMutableArray *tagStrings = [[NSMutableArray alloc] init];

	int tagCount = vorbis_comment_query_count(tags, [tag UTF8String]);

	for(int i = 0; i < tagCount; ++i) {
		const char *value = vorbis_comment_query(tags, [tag UTF8String], i);
		[tagStrings addObject:[NSString stringWithUTF8String:value]];
	}

	return [tagStrings componentsJoinedByString:@", "];
}

- (void)updateMetadata {
	vorbis_comment *tags = ov_comment(&vorbisRef, -1);

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

		size_t count = vorbis_comment_query_count(tags, "METADATA_BLOCK_PICTURE");
		if(count) {
			const char *pictureTag = vorbis_comment_query(tags, "METADATA_BLOCK_PICTURE", 0);
			flac_picture_t *picture = flac_picture_parse_from_base64(pictureTag);
			if(picture) {
				if(picture->binary && picture->binary_length) {
					_albumArt = [NSData dataWithBytes:picture->binary length:picture->binary_length];
				}
				flac_picture_free(picture);
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
	return @{ @"channels": @(channels),
		      @"bitsPerSample": @(32),
		      @"floatingPoint": @(YES),
		      @"sampleRate": @(frequency),
		      @"totalFrames": @(totalFrames),
		      @"bitrate": @(bitrate),
		      @"seekable": @([source seekable] && seekable),
		      @"codec": @"Ogg Vorbis",
		      @"endian": @"host",
		      @"encoding": @"lossy" };
}

- (NSDictionary *)metadata {
	return @{ @"artist": artist, @"albumartist": albumartist, @"album": album, @"title": title, @"genre": genre, @"year": year, @"track": track, @"disc": disc, @"albumArt": albumArt };
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
