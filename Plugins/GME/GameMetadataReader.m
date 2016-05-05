//
//  GameMetadataReader.m
//  GME
//
//  Created by Vincent Spader on 10/12/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "GameMetadataReader.h"

#import "GameDecoder.h"

#import <GME/gme.h>

#import "Logging.h"

@implementation GameMetadataReader

+ (NSArray *)fileTypes
{
	return [GameDecoder fileTypes];
}

+ (NSArray *)mimeTypes
{
	return [GameDecoder mimeTypes];
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

	NSString *ext = [[[url path] pathExtension] lowercaseString];
	
	gme_type_t type = gme_identify_extension([ext UTF8String]);
	if (!type) 
	{
		ALog(@"GME: No type!");
		return NO;
	}
	
	Music_Emu* emu;
	emu = gme_new_emu(type, gme_info_only);
	if (!emu)
	{
		ALog(@"GME: No new emu!");
		return NO;
	}
	
	[source seek:0 whence:SEEK_END];
	long size = [source tell];
	[source seek:0 whence:SEEK_SET];

	gme_err_t error;
	error = gme_load_custom(emu, readCallback, size, (__bridge void *)(source));
	if (error)
	{
		ALog(@"GME: ERROR Loding file!");
		return NO;
	}
    
	int track_num;
	if ([[url fragment] length] == 0)
		track_num = 0;
	else
		track_num = [[url fragment] intValue];
	
	gme_info_t * info;
	error = gme_track_info( emu, &info, track_num );
	if (error)
	{
		ALog(@"GME: Unable to get track info");
	}
	
	gme_delete(emu);

    NSString *title = [NSString stringWithUTF8String: info->song];
    if (!title || ![title length])
    {
        // this is needed to distinguish between different tracks in NSF, for example
        // otherwise they will all be displayed as 'blahblah.nsf' in playlist
        title = [[url lastPathComponent] stringByAppendingFormat:@" [%d]", track_num];
    }
    
    NSDictionary * dict = [NSDictionary dictionaryWithObjectsAndKeys:
		[NSString stringWithUTF8String: info->system], @"genre",
		[NSString stringWithUTF8String: info->game], @"album",
		title, @"title",
		[NSString stringWithUTF8String: info->author], @"artist",
		[NSNumber numberWithInt:track_num+1], @"track",
		nil];
    
    gme_free_info( info );
    
    return dict;
}

@end
