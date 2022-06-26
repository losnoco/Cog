//
//  PlaylistEntry.h
//  Cog
//
//  Created by Vincent Spader on 3/14/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "Cog-Swift.h"

@interface PlaylistEntry (Extension)

+ (NSSet *_Nonnull)keyPathsForValuesAffectingTitle;
+ (NSSet *_Nonnull)keyPathsForValuesAffectingDisplay;
+ (NSSet *_Nonnull)keyPathsForValuesAffectingLength;
+ (NSSet *_Nonnull)keyPathsForValuesAffectingPath;
+ (NSSet *_Nonnull)keyPathsForValuesAffectingFilename;
+ (NSSet *_Nonnull)keyPathsForValuesAffectingFilenameFragment;
+ (NSSet *_Nonnull)keyPathsForValuesAffectingStatus;
+ (NSSet *_Nonnull)keyPathsForValuesAffectingStatusMessage;
+ (NSSet *_Nonnull)keyPathsForValuesAffectingSpam;
+ (NSSet *_Nonnull)keyPathsForValuesAffectingIndexedSpam;
+ (NSSet *_Nonnull)keyPathsForValuesAffectingAlbumArt;
+ (NSSet *_Nonnull)keyPathsForValuesAffectingTrackText;
+ (NSSet *_Nonnull)keyPathsForValuesAffectingLengthText;
+ (NSSet *_Nonnull)keyPathsForValuesAffectingYearText;
+ (NSSet *_Nonnull)keyPathsForValuesAffectingCuesheetPresent;
+ (NSSet *_Nonnull)keyPathsForValuesAffectingGainCorrection;
+ (NSSet *_Nonnull)keyPathsForValuesAffectingUnsigned;

@property(nonatomic, readonly) NSString *_Nonnull display;
@property(nonatomic, retain, readonly) NSNumber *_Nonnull length;
@property(nonatomic, readonly) NSString *_Nonnull path;
@property(nonatomic, readonly) NSString *_Nonnull filename;
@property(nonatomic, readonly) NSString *_Nonnull filenameFragment;

@property(nonatomic, readonly) NSString *_Nonnull spam;
@property(nonatomic, readonly) NSString *_Nonnull indexedSpam;

@property(nonatomic, readonly) NSString *_Nonnull positionText;

@property(nonatomic, readonly) NSString *_Nonnull lengthText;

@property(nonatomic, readonly) NSString *_Nonnull yearText;

@property(nonatomic) NSString *_Nonnull title;

@property(nonatomic, readonly) NSString *_Nonnull trackText;

@property(nonatomic, readonly) NSString *_Nonnull cuesheetPresent;

@property(nonatomic, retain, readonly) NSImage *_Nullable albumArt;

@property(nonatomic, readonly) NSString *_Nonnull gainCorrection;

@property(nonatomic, readonly) NSString *_Nonnull gainInfo;

@property(nonatomic, readonly) NSString *_Nullable status;
@property(nonatomic, readonly) NSString *_Nullable statusMessage;

@property(nonatomic) NSURL *_Nullable url;
@property(nonatomic) NSURL *_Nullable trashUrl;

@property(nonatomic) NSData *_Nullable albumArtInternal;

@property(nonatomic) BOOL Unsigned;
@property(nonatomic) NSURL *_Nullable URL;

@property(nonatomic) PlayCount *_Nullable playCountItem;
@property(nonatomic, readonly) NSString *_Nonnull playCount;
@property(nonatomic, readonly) NSString *_Nonnull playCountInfo;

- (void)setMetadata:(NSDictionary *_Nonnull)metadata;

@end
