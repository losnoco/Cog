//
//  libvgmDecoder.h
//  libvgmPlayer
//
//  Created by Christopher Snowhill on 1/02/22.
//  Copyright 2022 __LoSnoCo__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <libvgm/utils/DataLoader.h>
#import <libvgm/player/playerbase.hpp>
#import <libvgm/player/playera.hpp>

#import "Plugin.h"

@interface libvgmDecoder : NSObject <CogDecoder> {
	UINT8* fileData;
	DATA_LOADER* dLoad;
	PlayerA* mainPlr;
	id<CogSource> source;
	long length;
	BOOL trackEnded;
}

- (BOOL)trackEnded;
- (void)setTrackEnded:(BOOL)ended;

- (void)setSource:(id<CogSource>)s;
- (id<CogSource>)source;

@end
