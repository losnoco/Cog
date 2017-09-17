//
//  VGMContainer.m
//  VGMStream
//
//  Created by Christopher Snowhill on 9/1/17.
//  Copyright 2017 __LoSnoCo__. All rights reserved.
//

#import "VGMContainer.h"
#import "VGMDecoder.h"
#import "VGMInterface.h"

#import "Logging.h"

@implementation VGMContainer

+ (NSArray *)fileTypes
{
    return [VGMDecoder fileTypes];
}

+ (NSArray *)mimeTypes 
{
	return nil;
}

+ (float)priority
{
    return [VGMDecoder priority];
}

+ (NSArray *)urlsForContainerURL:(NSURL *)url
{
    if ([url fragment]) {
        // input url already has fragment defined - no need to expand further
        return [NSMutableArray arrayWithObject:url];
    }

    VGMSTREAM * stream = init_vgmstream_from_cogfile([[[url absoluteString] stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding] UTF8String], 0);
	if (!stream)
	{
		ALog(@"Open failed for file: %@", [url absoluteString]);
		return NO;
	}
    
    NSMutableArray *tracks = [NSMutableArray array];
    
	int i;
    int subsongs = stream->num_streams;
    if (subsongs == 0)
        subsongs = 1;
    for (i = 1; i <= subsongs; ++i) {
        [tracks addObject:[NSURL URLWithString:[[url absoluteString] stringByAppendingFormat:@"#%i", i]]];
    }
    
	return tracks;
}


@end
