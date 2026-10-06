//
//  PlaylistControllerEnums.h
//  Cog
//
//  Created by Christopher Snowhill on 3/23/26.
//

#import <Foundation/Foundation.h>

typedef NS_ENUM(NSInteger, RepeatMode) {
    RepeatModeNoRepeat = 0,
    RepeatModeRepeatOne,
    RepeatModeRepeatAlbum,
    RepeatModeRepeatAll
};

typedef NS_ENUM(NSInteger, ShuffleMode) {
    ShuffleOff = 0,
    ShuffleAlbums,
    ShuffleAll }
;

typedef NS_ENUM(NSInteger, URLOrigin) {
    URLOriginInternal = 0,
    URLOriginExternal
};

/// Whether the playlist repeats the current track, which looping formats
/// (module, chiptune and MIDI decoders) honour by playing forever.
static inline BOOL IsRepeatOneSet(void) {
	return [[NSUserDefaults standardUserDefaults] integerForKey:@"repeat"] == RepeatModeRepeatOne;
}
