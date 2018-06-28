//
//  jxsMetadataReader.m
//  Syntrax-c
//
//  Created by Christopher Snowhill on 03/14/16.
//  Copyright 2016 __NoWork, Inc__. All rights reserved.
//

#import "jxsMetadataReader.h"
#import "jxsDecoder.h"

#import <Syntrax_c/syntrax.h>
#import <Syntrax_c/file.h>

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
	
    Song * synSong = File_loadSongMem(data, size);
    if (!synSong)
    {
        ALog(@"Open failed for file: %@", [url absoluteString]);
        free(data);
        return nil;
    }
    
    free(data);
    
    Player * synPlayer = playerCreate(44100);
    if (!synPlayer)
    {
        ALog(@"Failed to create player for file: %@", [url absoluteString]);
        File_freeSong(synSong);
        return nil;
    }
    
    if (loadSong(synPlayer, synSong) < 0)
    {
        ALog(@"Load failed for file: %@", [url absoluteString]);
        playerDestroy(synPlayer);
        File_freeSong(synSong);
        return nil;
    }
    
    int track_num;
    if ([[url fragment] length] == 0)
        track_num = 0;
    else
        track_num = [[url fragment] intValue];
    
    initSubsong(synPlayer, track_num);
    
    syntrax_info info;
    playerGetInfo(synPlayer, &info);
    
    playerDestroy(synPlayer);
    File_freeSong(synSong);
    
	//Some titles are all spaces?!
	NSString *title = [[NSString stringWithUTF8String: info.subsongName] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];

	if (title == nil) {
		title = @"";
	}
	
	return [NSDictionary dictionaryWithObject:title forKey:@"title"];
}

@end
