//
//  HLSSegmentManager.h
//  HLS
//
//  Created by Claude on 2026-05-05.
//  Mildly edited by Christopher Snowhill on 2026-05-05
//  Copyright 2026 __LoSnoCo__. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "Plugin.h"

@class HLSPlaylist;
@class HLSSegment;

@interface HLSSegmentManager : NSObject

@property (nonatomic, strong) HLSPlaylist *playlist;
@property (nonatomic) id<CogSource> source;
@property (nonatomic, strong) NSString *mimeType;
@property (nonatomic) NSInteger currentAbsoluteIndex;   // Absolute segment position in stream
@property (nonatomic) NSInteger nextAbsoluteIndex;      // Next absolute index to assign to new segments
@property (nonatomic) NSInteger bufferSize;
@property (nonatomic) NSInteger prefetchCount;

- (id)initWithPlaylist:(HLSPlaylist *)playlist;
- (NSData *)downloadSegment:(HLSSegment *)segment error:(NSError **)error;
- (BOOL)isSegmentBuffered:(NSInteger)index;
- (NSData *)getSegmentAtIndex:(NSInteger)index error:(NSError **)error;
- (BOOL)moveToNextSegment;
- (BOOL)hasSegmentAtIndex:(NSInteger)index;
- (NSInteger)segmentIndexForFrame:(long)frame;
- (long)offsetInFrame:(long)frame forSegment:(NSInteger)segmentIndex;
- (BOOL)needsMoreSegments;
- (void)refreshPlaylist:(NSError **)error;
- (double)totalDuration;

@end
