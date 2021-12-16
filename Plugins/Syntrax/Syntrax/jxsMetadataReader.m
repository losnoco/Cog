//
//  jxsMetadataReader.m
//  Syntrax-c
//
//  Created by Christopher Snowhill on 03/14/16.
//  Copyright 2016 __NoWork, Inc__. All rights reserved.
//

#import "jxsMetadataReader.h"
#import "jxsDecoder.h"

#import <Syntrax_c/jxs.h>
#import <Syntrax_c/jaytrax.h>

#import "Logging.h"

@implementation jxsMetadataReader

+ (NSArray *)fileTypes
{
	return [jxsDecoder fileTypes];
}

+ (NSArray *)mimeTypes
{
	return [jxsDecoder mimeTypes];
}

+ (float)priority
{
    return 1.0f;
}

+ (NSDictionary *)metadataForURL:(NSURL *)url
{
    id audioSourceClass = NSClassFromString(@"AudioSource");
    id<CogSource> source = [audioSourceClass audioSourceForURL:url];
    
    if (![source open:url])
        return 0;
    
    if (![source seekable])
        return 0;

    [source seek:0 whence:SEEK_END];
    long size = [source tell];
    [source seek:0 whence:SEEK_SET];
    
    void * data = malloc(size);
    [source read:data amount:size];
	
    JT1Song* synSong;
    if (jxsfile_readSongMem(data, size, &synSong))
    {
        ALog(@"Open failed for file: %@", [url absoluteString]);
        free(data);
        return nil;
    }
    
    free(data);
    
    JT1Player * synPlayer = jaytrax_init();
    if (!synPlayer)
    {
        ALog(@"Failed to create player for file: %@", [url absoluteString]);
        jxsfile_freeSong(synSong);
        return nil;
    }
    
    if (!jaytrax_loadSong(synPlayer, synSong))
    {
        ALog(@"Load failed for file: %@", [url absoluteString]);
        jaytrax_free(synPlayer);
        jxsfile_freeSong(synSong);
        return nil;
    }
    
    int track_num;
    if ([[url fragment] length] == 0)
        track_num = 0;
    else
        track_num = [[url fragment] intValue];
    
    jaytrax_changeSubsong(synPlayer, track_num);
    
	//Some titles are all spaces?!
	NSString *title = [[NSString stringWithUTF8String: synPlayer->subsong->name] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
    
    jaytrax_free(synPlayer);
    jxsfile_freeSong(synSong);

	if (title == nil) {
		title = @"";
	}
	
	return [NSDictionary dictionaryWithObject:title forKey:@"title"];
}

@end
