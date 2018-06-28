//
//  jxsContainer.m
//  Syntrax-c
//
//  Created by Christopher Snowhill on 03/14/16.
//  Copyright 2016 __NoWork, Inc__. All rights reserved.
//

#import <Syntrax_c/syntrax.h>
#import <Syntrax_c/file.h>

#import "jxsContainer.h"
#import "jxsDecoder.h"

#import "Logging.h"

@implementation jxsContainer

+ (NSArray *)fileTypes
{
    return [jxsDecoder fileTypes];
}

+ (NSArray *)mimeTypes 
{
	return nil;
}

+ (float)priority
{
    return 1.0f;
}

+ (NSArray *)urlsForContainerURL:(NSURL *)url
{
    if ([url fragment]) {
        // input url already has fragment defined - no need to expand further
        return [NSMutableArray arrayWithObject:url];
    }
    
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
        ALog(@"Failed to create Syntrax-c player for file: %@", [url absoluteString]);
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
    
    NSMutableArray *tracks = [NSMutableArray array];
    
    syntrax_info info;
    
    playerGetInfo(synPlayer, &info);
    
    playerDestroy(synPlayer);
    File_freeSong(synSong);
    
	int i;
    int subsongs = info.totalSubs;
    if ( subsongs ) {
        for (i = 0; i < subsongs; i++)
        {
            [tracks addObject:[NSURL URLWithString:[[url absoluteString] stringByAppendingFormat:@"#%i", i]]];
        }
    }
    
	return tracks;
}


@end
