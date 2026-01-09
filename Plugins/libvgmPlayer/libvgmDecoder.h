//
//  libvgmDecoder.h
//  libvgmPlayer
//
//  Created by Christopher Snowhill on 1/02/22.
//  Copyright 2022-2026 __LoSnoCo__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <libvgm/player/playera.hpp>
#import <libvgm/player/playerbase.hpp>
#import <libvgm/utils/DataLoader.h>

#import "Plugin.h"

@interface libvgmDecoder : NSObject <CogDecoder> {
	UINT8* fileData;
	DATA_LOADER* dLoad;
	PlayerA* mainPlr;
	id<CogSource> source;
	UINT8* sampleBuffer;
	double sampleRate;
	long loopCount;
	double fadeTime;
	long length;
	BOOL trackEnded;
}

- (BOOL)trackEnded;
- (void)setTrackEnded:(BOOL)ended;

- (void)setSource:(id<CogSource>)s;
- (id<CogSource>)source;

@end
