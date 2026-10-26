//
//  DumbFile.h
//  Cog
//
//  Created by Vincent Spader on 5/29/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import <Foundation/Foundation.h>

#import <sidplayfp/SidConfig.h>
#import <sidplayfp/SidInfo.h>
#import <sidplayfp/SidTune.h>
#import <sidplayfp/SidTuneInfo.h>
#import <sidplayfp/Event.h>
#import <sidplayfp/sidbuilder.h>
#import <sidplayfp/sidplayfp.h>

#import "Plugin.h"

@interface SidDecoder : NSObject <CogDecoder> {
	SidTune *tune;
	sidplayfp *engine;
	sidbuilder *builder;

	id<CogSource> source;
	long length;

	NSString *currentUrl;
	BOOL hintAdded;

	double sampleRate;

	int n_channels;

	long renderedTotal;
	long fadeTotal;
	long fadeRemain;
}

- (void)setSource:(id<CogSource>)s;
- (id<CogSource>)source;
- (void)cleanUp;

// Whether a tune at `url` is loaded with its stereo companion file (.mus
// with .str), so its own data alone doesn't describe how it plays.
+ (BOOL)loadsCompanionForURL:(NSURL *)url;
// The rate tunes play at, and how many frames before the fade, as set.
+ (double)sampleRate;
+ (long)lengthForSampleRate:(double)sampleRate;
// Sets up a new engine and builder to play `tune` at `sampleRate`, and the
// channels it renders. NO if the tune can't play; whatever was made is left
// in *engine and *builder for the caller to delete. May throw.
+ (BOOL)loadTune:(SidTune *)tune sampleRate:(double)sampleRate engine:(sidplayfp **)engine builder:(sidbuilder **)builder channels:(int *)channels;
+ (NSDictionary *)propertiesWithSampleRate:(double)sampleRate length:(long)length channels:(int)channels;
@end
