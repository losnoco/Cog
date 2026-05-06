//
//  HLSVariant.h
//  HLS
//
//  Created by Claude on 2026-05-05.
//  Copyright 2026 __LoSnoCo__. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface HLSVariant : NSObject

@property (nonatomic) NSInteger bandwidth;
@property (nonatomic) NSInteger averageBandwidth;
@property (nonatomic, strong) NSString *codecs;
@property (nonatomic, strong) NSString *resolution;
@property (nonatomic, strong) NSURL *url;
@property (nonatomic, strong) NSURL *playlistURL;
@property (nonatomic, strong) NSDictionary *audioGroups;
@property (nonatomic, strong) NSDictionary *subtitleGroups;

@end
