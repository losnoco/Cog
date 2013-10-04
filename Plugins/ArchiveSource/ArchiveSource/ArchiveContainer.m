//
//  CueSheetContainer.m
//  CueSheet
//
//  Created by Zaphod Beeblebrox on 10/8/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "ArchiveContainer.h"

#import <File_Extractor/fex.h>

static NSString * path_pack_string(NSString * src)
{
    return [NSString stringWithFormat:@"|%lu|%@|", [src length], src];
}

static NSString * g_make_unpack_path(NSString * archive, NSString * file, NSString * name)
{
    return [NSString stringWithFormat:@"unpack://%@%@%@", name, path_pack_string(archive), file];
}

@implementation ArchiveContainer

+ (NSArray *)fileTypes
{
	return [NSArray arrayWithObjects:@"zip", @"rar", @"7z", @"rsn", @"vgm7z", @"gz", @"vgm", @"vgz", nil];
}

+ (NSArray *)mimeTypes
{
	return [NSArray arrayWithObjects:@"application/zip", @"application/x-gzip", @"application/x-rar-compressed", @"application/x-7z-compressed", nil];
}

+ (void)initialize
{
    fex_init();
}

+ (NSArray *)urlsForContainerURL:(NSURL *)url
{
	if (![url isFileURL]) {
		return [NSArray array];
	}
	
    fex_t * fex;
    fex_err_t error = fex_open( &fex, [[url path] UTF8String] );
    if ( error ) {
        NSLog(@"Archive error: %s", error);
        return [NSArray array];
    }
    
	NSMutableArray *files = [NSMutableArray array];
	
    while ( !fex_done(fex) ) {
        [files addObject:[NSURL URLWithString:[g_make_unpack_path([url path], [NSString stringWithUTF8String:fex_name(fex)], @"fex") stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding]]];
        fex_next(fex);
    }
    
    fex_close( fex );
	
	return files;
}

@end
