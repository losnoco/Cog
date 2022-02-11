//
//  DumbFile.h
//  Cog
//
//  Created by Vincent Spader on 5/29/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <sidplayfp/SidConfig.h>
#import <sidplayfp/SidInfo.h>
#import <sidplayfp/SidTune.h>
#import <sidplayfp/SidTuneInfo.h>
#import <sidplayfp/event.h>
#import <sidplayfp/sidbuilder.h>
#import <sidplayfp/sidplayfp.h>

#import "Plugin.h"

@interface SidDecoder : NSObject <CogDecoder> {
	SidTune *tune;
	sidplayfp *engine;
	sidbuilder *builder;

	long length;

	NSString *currentUrl;
	BOOL hintAdded;

	int n_channels;

	long renderedTotal;
	long fadeTotal;
	long fadeRemain;
}

- (void)cleanUp;
@end
