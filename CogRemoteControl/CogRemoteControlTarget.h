//
//  CogRemoteControlTarget.h
//  CogRemoteControl
//
//  Created by Kevin López Brante on 2026-07-01.
//
//  The boundary between the MCP server framework and the host app, kept in
//  Objective-C so the 10.15 app can compile against this 13.0-only framework
//  (Swift module imports enforce the deployment target; ObjC headers don't).
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

API_AVAILABLE(macos(13.0))
NS_SWIFT_UI_ACTOR
NS_SWIFT_SENDABLE
@protocol CogRemoteControlTarget <NSObject>

- (void)remotePlay;
- (void)remotePause;
- (void)remoteResume;
- (void)remoteStop;
- (void)remoteNext;
- (void)remotePrevious;

/// nil when stopped; otherwise keys: title, artist, album (NSString),
/// position, length (NSNumber seconds), status (@"playing"/@"paused"),
/// seekable (NSNumber bool). Values may be missing when unknown.
- (nullable NSDictionary<NSString *, id> *)remoteNowPlaying;

/// Volume on the linear 0-100 scale used by the volume slider.
- (double)remoteVolume;
- (void)remoteSetVolume:(double)volume;

/// nil when stopped; otherwise seconds into the current track.
- (nullable NSNumber *)remotePosition;
- (void)remoteSeekTo:(double)position NS_SWIFT_NAME(remoteSeek(to:));
- (BOOL)remoteIsSeekable;

/// Modes: @"off", @"albums", @"all".
- (NSString *)remoteShuffleMode;
- (void)remoteSetShuffleMode:(NSString *)mode;

/// Modes: @"none", @"one", @"album", @"all".
- (NSString *)remoteRepeatMode;
- (void)remoteSetRepeatMode:(NSString *)mode;

/// Array of dictionaries with keys: index (NSNumber), title, artist, album
/// (NSString), length (NSNumber seconds), url (NSString), queued, current
/// (NSNumber bool). Values may be missing when unknown.
- (NSArray<NSDictionary<NSString *, id> *> *)remoteListPlaylist;

/// Returns the number of entries added.
- (NSInteger)remoteAddToPlaylist:(NSArray<NSString *> *)urls NS_SWIFT_NAME(remoteAdd(toPlaylist:));

- (BOOL)remoteRemoveFromPlaylistAtIndex:(NSInteger)index NS_SWIFT_NAME(remoteRemoveFromPlaylist(at:));
- (BOOL)remotePlayPlaylistEntryAtIndex:(NSInteger)index NS_SWIFT_NAME(remotePlayPlaylistEntry(at:));

/// Returns the entry's new queued state, or nil if the index is invalid.
- (nullable NSNumber *)remoteToggleQueueAtIndex:(NSInteger)index NS_SWIFT_NAME(remoteToggleQueue(at:));

@end

NS_ASSUME_NONNULL_END
