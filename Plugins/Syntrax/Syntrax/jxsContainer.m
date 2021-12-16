//
//  jxsContainer.m
//  Syntrax-c
//
//  Created by Christopher Snowhill on 03/14/16.
//  Copyright 2016 __NoWork, Inc__. All rights reserved.
//

#import <Syntrax_c/jxs.h>
#import <Syntrax_c/jaytrax.h>

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

    JT1Song * synSong;
    if (jxsfile_readSongMem(data, size, &synSong))
    {
        ALog(@"Open failed for file: %@", [url absoluteString]);
        free(data);
        return nil;
    }
    
    free(data);
    
    int i;
    int subsongs = synSong->nrofsongs;
    
    jxsfile_freeSong(synSong);

    NSMutableArray *tracks = [NSMutableArray array];
    
    if ( subsongs ) {
        for (i = 0; i < subsongs; i++)
        {
            [tracks addObject:[NSURL URLWithString:[[url absoluteString] stringByAppendingFormat:@"#%i", i]]];
        }
    }
    
	return tracks;
}


@end
