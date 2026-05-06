//
//  HLSPlaylist.h
//  HLS
//
//  Created by Claude on 2026-05-05.
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

@property (nonatomic, strong) NSURL *url;
@property (nonatomic) BOOL isMasterPlaylist;
@property (nonatomic) BOOL isLiveStream;
@property (nonatomic) NSInteger version;
@property (nonatomic) NSInteger targetDuration;
@property (nonatomic) NSInteger mediaSequence;
@property (nonatomic, strong) NSArray<HLSSegment *> *segments;
@property (nonatomic, strong) NSArray<HLSVariant *> *variants;
@property (nonatomic, strong) NSURL *baseURL;
@property (nonatomic) HLSPlaylistType type;

@end
