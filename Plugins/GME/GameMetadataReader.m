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

@implementation GameMetadataReader

+ (NSArray *)fileTypes
{
	return [GameDecoder fileTypes];
}

+ (NSArray *)mimeTypes
{
	return [GameDecoder mimeTypes];
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
		NSLog(@"No type!");
		return NO;
	}
	
	Music_Emu* emu;
	emu = gme_new_emu(type, gme_info_only);
	if (!emu)
	{
		NSLog(@"No new emu!");
		return NO;
	}
	
	[source seek:0 whence:SEEK_END];
	long size = [source tell];
	[source seek:0 whence:SEEK_SET];

	gme_err_t error;
	error = gme_load_custom(emu, readCallback, size, source);
	if (error)
	{
		NSLog(@"ERROR Loding file!");
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
		NSLog(@"Unable to get track info");
	}
	
	gme_delete(emu);

	NSDictionary * dict = [NSDictionary dictionaryWithObjectsAndKeys:
		[NSString stringWithUTF8String: info->system], @"genre",
		[NSString stringWithUTF8String: info->game], @"album",
		[NSString stringWithUTF8String: info->song], @"title",
		[NSString stringWithUTF8String: info->author], @"artist",
		[NSNumber numberWithInt:track_num+1], @"track",
		nil];
    
    gme_free_info( info );
    
    return dict;
}

@end
