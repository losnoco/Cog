//
//  PlaylistEntry.h
//  Cog
//
//  Created by Vincent Spader on 3/14/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface PlaylistEntry : NSObject {
	int index;
	int shuffleIndex;
	
	BOOL current;
	BOOL removed;
	
	BOOL stopAfter;
	
	BOOL queued;
	int queuePosition;
	
	BOOL error;
	NSString *errorMessage;
	
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
	
	NSString *endian;
	
	BOOL seekable;
}

+ (NSSet *)keyPathsForValuesAffectingDisplay;
+ (NSSet *)keyPathsForValuesAffectingLength;
+ (NSSet *)keyPathsForValuesAffectingPath;
+ (NSSet *)keyPathsForValuesAffectingFilename;
+ (NSSet *)keyPathsForValuesAffectingStatus;
+ (NSSet *)keyPathsForValuesAffectingStatusMessage;

@property(readonly) NSString *display;
@property(retain, readonly) NSNumber *length;
@property(readonly) NSString *path;
@property(readonly) NSString *filename;

@property int index;
@property int shuffleIndex;

@property(readonly) NSString *status;
@property(readonly) NSString *statusMessage;

@property BOOL current;
@property BOOL removed;

@property BOOL stopAfter;

@property BOOL queued;
@property int queuePosition;

@property BOOL error;
@property(retain) NSString *errorMessage;

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

@property(retain) NSString *endian;

@property BOOL seekable;

@end
