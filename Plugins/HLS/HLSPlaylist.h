//
//  HLSPlaylist.h
//  HLS
//
//  Created by Claude on 2026-05-05.
//  Rewritten by Christopher Snowhill on 2026-05-06
//  Copyright 2026 __LoSnoCo__. All rights reserved.
//

#import <Foundation/Foundation.h>

@class HLSSegment;
@class HLSVariant;

typedef NS_ENUM(NSInteger, HLSPlaylistType) {
	HLSPlaylistTypeUnspecified = 0,
	HLSPlaylistTypeEvent = 1,
	HLSPlaylistTypeVOD = 2
};

@interface HLSPlaylist : NSObject

// The original URL we fetched the playlist from. After redirects this may
// differ from the URL we started with -- relative segment URIs should be
// resolved against this.
@property (nonatomic, strong) NSURL *url;

@property (nonatomic) BOOL isMasterPlaylist;
@property (nonatomic) BOOL isLiveStream;        // YES until ENDLIST or PLAYLIST-TYPE:VOD seen
@property (nonatomic) BOOL hasEndList;
@property (nonatomic) NSInteger version;
@property (nonatomic) NSInteger targetDuration;
@property (nonatomic) NSInteger mediaSequence;
@property (nonatomic) NSInteger discontinuitySequence;
@property (nonatomic) HLSPlaylistType type;

@property (nonatomic, strong) NSArray<HLSSegment *> *segments;
@property (nonatomic, strong) NSArray<HLSVariant *> *variants;

@end
