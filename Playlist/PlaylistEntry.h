//
//  PlaylistEntry.h
//  Cog
//
//  Created by Vincent Spader on 3/14/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "SoundFile.h"

@interface PlaylistEntry : NSObject {
	NSString *filename;
	NSString *display;

	NSString *artist;
	NSString *album;
	NSString *title;
	NSString *genre;

	NSString *year;
	unsigned int track;	
	
	NSString *lengthString;
	
	double length;
	int bitrate;
	int channels;
	int bitsPerSample;
	float sampleRate;
	
	BOOL current;
	
	int idx; //Can't use index due to some weird bug...might be fixed...should test in the future...think it was a conflict with flac, which is now an external lib
	int shuffleIdx;
	int displayIdx;
}

-(void)setIndex:(int)i;
-(int)index;

-(void)setShuffleIndex:(int)si;
-(int)shuffleIndex;

-(void)setFilename:(NSString *)f;
-(NSString *)filename;
-(void)setDisplay:(NSString *)d;
-(NSString *)display;
-(void)setCurrent:(BOOL) b;
-(BOOL)current;

- (void)setArtist:(NSString *)s;
- (NSString *)artist;
- (void)setAlbum:(NSString *)s;
- (NSString *)album;
- (void)setTitle:(NSString *)s;
- (NSString *)title;
- (void)setGenre:(NSString *)s;
- (NSString *)genre;

- (NSString *)lengthString;
- (void)setLengthString:(double)l;

-(void)setYear:(NSString *)y;
-(NSString *)year;
-(void)setTrack:(int)y;
-(int)track;

- (double)length;
- (int)bitrate;
- (int)channels;
- (int)bitsPerSample;
- (float)sampleRate;

- (void)readTags;
- (void)readTagsThreaded;
- (void)readInfo;
- (void)readInfoThreaded;

@end
