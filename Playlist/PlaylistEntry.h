//
//  PlaylistEntry.h
//  Cog
//
//  Created by Vincent Spader on 3/14/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <CogPlaylist/CogPlaylist-Swift.h>

#import "Cog-Swift.h"

// Posted (object: the entry) once an entry's metadata has been loaded, which
// for a track that started playing first can bring its ReplayGain late.
extern NSNotificationName _Nonnull const CogPlaylistEntryMetadataLoadedNotification;

@interface PlaylistEntry (Extension)

+ (NSSet *_Nonnull)keyPathsForValuesAffectingStatusMessage;
+ (NSSet *_Nonnull)keyPathsForValuesAffectingSpam;
+ (NSSet *_Nonnull)keyPathsForValuesAffectingIndexedSpam;
+ (NSSet *_Nonnull)keyPathsForValuesAffectingAlbumArt;
+ (NSSet *_Nonnull)keyPathsForValuesAffectingLengthText;
+ (NSSet *_Nonnull)keyPathsForValuesAffectingLengthInfo;
+ (NSSet *_Nonnull)keyPathsForValuesAffectingGainCorrection;

@property(nonatomic, readonly) NSString *_Nonnull spam;
@property(nonatomic, readonly) NSString *_Nonnull indexedSpam;

@property(nonatomic, readonly) NSString *_Nonnull positionText;

@property(nonatomic, readonly) NSString *_Nonnull lengthText;
@property(nonatomic, readonly) NSString *_Nonnull lengthInfo;

@property(nonatomic, retain, readonly) NSImage *_Nullable albumArt;

@property(nonatomic, readonly) NSString *_Nonnull gainCorrection;

@property(nonatomic, readonly) NSString *_Nonnull gainInfo;

@property(nonatomic, readonly) NSString *_Nullable statusMessage;

@property(nonatomic) NSData *_Nullable urlBookmark;

@property(nonatomic) NSData *_Nullable albumArtInternal;

@property(nonatomic) PlayCount *_Nullable playCountItem;
@property(nonatomic, readonly) NSString *_Nonnull playCount;
@property(nonatomic, readonly) NSString *_Nonnull playCountInfo;

@property(nonatomic, readonly) float rating;

- (void)setMetadata:(NSDictionary *_Nonnull)metadata;

- (AudioScrobblerTrack *_Nonnull)audioScrobblerTrack;

@end
