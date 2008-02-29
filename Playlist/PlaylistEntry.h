//
//  PlaylistEntry.h
//  Cog
//
//  Created by Vincent Spader on 3/14/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>

typedef enum {
	kCogEntryNormal = 0,
	kCogEntryPlaying,
	kCogEntryError,
	kCogEntryQueued,
	kCogEntryRemoved,
	kCogEntryStoppingAfterCurrent,
} PlaylistEntryStatus;

@interface PlaylistEntry : NSObject {
	int index;
	int shuffleIndex;
	PlaylistEntryStatus status;
	NSString *statusMessage;
	int queuePosition;
	
	NSURL *URL;
	
	NSString *artist;
	NSString *album;
	NSString *title;
	NSString *genre;
	NSString *year;
	NSNumber *track;
	
	long long totalFrames;
	int bitrate;
	int channels;
	int bitsPerSample;
	float sampleRate;
	
	BOOL seekable;
}

- (void)setMetadata: (NSDictionary *)m;
- (void)readMetadataThread;
- (void)setProperties: (NSDictionary *)p;
- (void)readPropertiesThread;

@property(readonly) NSString *display;
@property(retain, readonly) NSNumber *length;
@property(readonly) NSString *path;
@property(readonly) NSString *filename;

@property int index;
@property int shuffleIndex;
@property PlaylistEntryStatus status;
@property(retain) NSString *statusMessage;
@property int queuePosition;
@property(retain) NSURL *URL;

@property(retain) NSString *artist;
@property(retain) NSString *album;
@property(retain) NSString *title;
@property(retain) NSString *genre;
@property(retain) NSString *year;
@property(retain) NSNumber *track;

@property long long totalFrames;
@property int bitrate;
@property int channels;
@property int bitsPerSample;
@property float sampleRate;

@property BOOL seekable;

@end
