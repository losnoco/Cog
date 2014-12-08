//
//  DumbFile.h
//  Cog
//
//  Created by Vincent Spader on 5/29/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <sidplayfp/SidTune.h>
#import <sidplayfp/sidplayfp.h>
#import <sidplayfp/event.h>
#import <sidplayfp/SidConfig.h>
#import <sidplayfp/sidbuilder.h>
#import <sidplayfp/SidInfo.h>
#import <sidplayfp/SidTuneInfo.h>

#import "Plugin.h"

@interface SidDecoder : NSObject <CogDecoder> {
    SidTune *tune;
    sidplayfp *engine;
    sidbuilder *builder;
    
	id<CogSource> source;
	long length;
    
    int n_channels;
    
    long renderedTotal;
    long fadeTotal;
    long fadeRemain;
}

- (void)setSource:(id<CogSource>)s;
- (id<CogSource>)source;
- (void)cleanUp;
@end
