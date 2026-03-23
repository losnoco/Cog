//
//  PlaylistControllerEnums.h
//  Cog
//
//  Created by Christopher Snowhill on 3/23/26.
//


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
