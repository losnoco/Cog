//
//  GameFile.h
//  Cog
//
//  Created by Vincent Spader on 5/29/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import <Foundation/Foundation.h>

#import <GME/gme.h>

#import "Plugin.h"

extern gme_err_t readCallback(void* data, void* out, int count);

@interface GameDecoder : NSObject <CogDecoder> {
	Music_Emu* emu;
	id<CogSource> source;
	double sampleRate;
	long length;
	long fade;
	NSString* codec;
	
	int16_t sampleBuffer[1024 * 2];
}

- (void)setSource:(id<CogSource>)s;
- (id<CogSource>)source;

// The rate a file of `type` plays at, and a track's length, fade included,
// and its fade, in milliseconds, as the decoder plays them.
+ (double)sampleRateForType:(gme_type_t)type;
+ (void)getLength:(long *)length fade:(long *)fade forTrackInfo:(gme_info_t *)info;
// The properties of a track that plays `length` milliseconds at
// `sampleRate`.
+ (NSDictionary *)propertiesWithSampleRate:(double)sampleRate length:(long)length;
// Loads the m3u playlist named after the file at `url`, or, failing that,
// the lines naming the file in the playlists beside it, as rips that give
// each track its own playlist have them.
+ (void)loadPlaylistForURL:(NSURL *)url intoEmu:(Music_Emu *)emu;

@end
