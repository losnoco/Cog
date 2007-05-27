//
//  PlaylistEntry.h
//  Cog
//
//  Created by Vincent Spader on 3/14/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface PlaylistEntry : NSObject {
	NSURL *url;

	NSString *artist;
	NSString *album;
	NSString *title;
	NSString *genre;

	NSString *year;
	NSNumber *track;	
	
	NSNumber *length;
	NSNumber *bitrate;
	NSNumber *channels;
	NSNumber *bitsPerSample;
	NSNumber *sampleRate;
	
	NSNumber *current;
	
	NSNumber *idx;
	NSNumber *shuffleIndex;
	
	NSNumber *seekable;
}

- (void)setIndex:(NSNumber *)i;
- (NSNumber *)index;

- (void)setShuffleIndex:(NSNumber *)si;
- (NSNumber *)shuffleIndex;

- (void)setURL:(NSURL *)u;
- (NSURL *)url;
- (void)setCurrent:(NSNumber *) b;
- (NSNumber *)current;

- (void)setArtist:(NSString *)s;
- (NSString *)artist;
- (void)setAlbum:(NSString *)s;
- (NSString *)album;
- (void)setTitle:(NSString *)s;
- (NSString *)title;
- (void)setGenre:(NSString *)s;
- (NSString *)genre;

- (void)setYear:(NSString *)y;
- (NSString *)year;
- (void)setTrack:(NSNumber *)y;
- (NSNumber *)track;

- (void)setLength:(NSNumber *)l;
- (NSNumber *)length;

- (void)setBitrate:(NSNumber *) br;
- (NSNumber *)bitrate;

- (void)setChannels:(NSNumber *)c;
- (NSNumber *)channels;
- (void)setBitsPerSample:(NSNumber *)bps;
- (NSNumber *)bitsPerSample;
- (void)setSampleRate:(NSNumber *)s;
- (NSNumber *)sampleRate;

- (void)setSeekable:(NSNumber *)s;
- (NSNumber *)seekable;

- (void)setMetadata: (NSDictionary *)m;
- (void)readMetadataThread;
- (void)setProperties: (NSDictionary *)p;
- (void)readPropertiesThread;

@end
