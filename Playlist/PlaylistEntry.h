//
//  PlaylistEntry.h
//  Cog
//
//  Created by Vincent Spader on 3/14/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface PlaylistEntry : NSObject <NSCopying> {
	NSInteger index;
	NSInteger shuffleIndex;
	NSInteger dbIndex;
	NSInteger entryId;

	BOOL current;
	BOOL removed;

	BOOL stopAfter;

	BOOL queued;
	NSInteger queuePosition;

	BOOL error;
	NSString *errorMessage;

	NSURL *URL;
	NSURL *trashURL;

	NSString *artist;
	NSString *albumartist;
	NSString *album;
	NSString *title;
	NSString *genre;
	NSNumber *year;
	NSNumber *track;
	NSNumber *disc;

	NSString *cuesheet;

	NSData *albumArtInternal;

	float replayGainAlbumGain;
	float replayGainAlbumPeak;
	float replayGainTrackGain;
	float replayGainTrackPeak;
	float volume;

	double currentPosition;

	long long totalFrames;
	int bitrate;
	int channels;
	uint32_t channelConfig;
	int bitsPerSample;
	BOOL floatingPoint;
	BOOL Unsigned;
	float sampleRate;

	NSString *codec;

	NSString *endian;

	NSString *encoding;

	BOOL seekable;

	BOOL metadataLoaded;

	BOOL deleted;
}

+ (NSSet *)keyPathsForValuesAffectingDisplay;
+ (NSSet *)keyPathsForValuesAffectingLength;
+ (NSSet *)keyPathsForValuesAffectingPath;
+ (NSSet *)keyPathsForValuesAffectingFilename;
+ (NSSet *)keyPathsForValuesAffectingStatus;
+ (NSSet *)keyPathsForValuesAffectingStatusMessage;
+ (NSSet *)keyPathsForValuesAffectingSpam;
+ (NSSet *)keyPathsForValuesAffectingAlbumArt;
+ (NSSet *)keyPathsForValuesAffectingTrackText;
+ (NSSet *)keyPathsForValuesAffectingLengthText;
+ (NSSet *)keyPathsForValuesAffectingYearText;
+ (NSSet *)keyPathsForValuesAffectingCuesheetPresent;
+ (NSSet *)keyPathsForValuesAffectingGainCorrection;

@property(readonly) NSString *display;
@property(retain, readonly) NSNumber *length;
@property(readonly) NSString *path;
@property(readonly) NSString *filename;

@property(readonly) NSString *spam;

@property(readonly) NSString *positionText;

@property(readonly) NSString *lengthText;

@property(readonly) NSString *yearText;

@property(readonly) NSString *rawTitle;

@property(readonly) NSString *trackText;

@property NSInteger index;
@property NSInteger shuffleIndex;
@property NSInteger dbIndex;
@property NSInteger entryId;

@property(readonly) NSString *status;
@property(readonly) NSString *statusMessage;

@property BOOL current;
@property BOOL removed;

@property BOOL stopAfter;

@property BOOL queued;
@property NSInteger queuePosition;

@property BOOL error;
@property(retain) NSString *errorMessage;

@property(retain) NSURL *URL;
@property(retain) NSURL *trashURL;

@property(retain) NSString *artist;
@property(retain) NSString *albumartist;
@property(retain) NSString *album;
@property(nonatomic, retain) NSString *title;
@property(retain) NSString *genre;
@property(retain) NSNumber *year;
@property(retain) NSNumber *track;
@property(retain) NSNumber *disc;

@property(retain) NSString *cuesheet;

@property(readonly) NSString *cuesheetPresent;

@property(retain, readonly) NSImage *albumArt;
@property(retain) NSData *albumArtInternal;

@property long long totalFrames;
@property int bitrate;
@property int channels;
@property uint32_t channelConfig;
@property int bitsPerSample;
@property BOOL floatingPoint;
@property BOOL Unsigned;
@property float sampleRate;

@property(retain) NSString *codec;

@property float replayGainAlbumGain;
@property float replayGainAlbumPeak;
@property float replayGainTrackGain;
@property float replayGainTrackPeak;
@property float volume;

@property(readonly) NSString *gainCorrection;

@property(readonly) NSString *gainInfo;

@property double currentPosition;

@property(retain) NSString *endian;

@property(retain) NSString *encoding;

@property BOOL seekable;

@property BOOL metadataLoaded;

@property BOOL deleted;

- (void)setMetadata:(NSDictionary *)metadata;

@end
