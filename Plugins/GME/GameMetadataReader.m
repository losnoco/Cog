//
//  GameMetadataReader.m
//  GME
//
//  Created by Vincent Spader on 10/12/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "GameMetadataReader.h"

#import "GameContainer.h"

#import <GME/gme.h>

@implementation GameMetadataReader

+ (NSArray *)fileTypes
{
	return [GameContainer fileTypes];
}

+ (NSArray *)mimeTypes
{
	return [GameContainer mimeTypes];
}

+ (NSDictionary *)metadataForURL:(NSURL *)url
{
	if (![url isFileURL])
		return nil;

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
	
	gme_err_t error;
	error = gme_load_file(emu, [[url path] UTF8String]);
	if (error) 
	{
		NSLog(@"ERROR Loding file!");
		return NO;
	}
	
	int track_num = [[url fragment] intValue]; //What if theres no fragment? Assuming we get 0.
	
	track_info_t info;
	error = gme_track_info( emu, &info, track_num );
	if (error)
	{
		NSLog(@"Unable to get track info");
	}
	
	gme_delete(emu);

	return [NSDictionary dictionaryWithObjectsAndKeys:
		[NSString stringWithUTF8String: info.system], @"genre",
		[NSString stringWithUTF8String: info.game], @"album",
		[NSString stringWithUTF8String: info.song], @"title",
		[NSString stringWithUTF8String: info.author], @"artist",
		[NSNumber numberWithInt:track_num], @"track",
		nil];
}

@end
