//
//  VGMMetadataReader.m
//  VGMStream
//
//  Created by Christopher Snowhill on 9/18/17.
//  Copyright 2017 __LoSnoCo__. All rights reserved.
//

#import "VGMMetadataReader.h"
#import "VGMDecoder.h"
#import "VGMInterface.h"

@implementation VGMMetadataReader

+ (NSArray *)fileTypes
{
	return [VGMDecoder fileTypes];
}

+ (NSArray *)mimeTypes
{
	return [VGMDecoder mimeTypes];
}

+ (float)priority
{
    return [VGMDecoder priority];
}

+ (NSDictionary *)metadataForURL:(NSURL *)url
{
    int track_num = [[url fragment] intValue];
    
    NSString * path = [url absoluteString];
    NSRange fragmentRange = [path rangeOfString:@"#" options:NSBackwardsSearch];
    if (fragmentRange.location != NSNotFound) {
        path = [path substringToIndex:fragmentRange.location];
    }
    
    VGMSTREAM * stream = init_vgmstream_from_cogfile([[path stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding] UTF8String], track_num);
    if ( !stream )
        return nil;
    
    NSString * title;
    
    if ( stream->num_streams > 1 ) {
        title = [NSString stringWithFormat:@"%@ - %s", [[url URLByDeletingPathExtension] lastPathComponent], stream->stream_name];
    } else {
        title = [[url URLByDeletingPathExtension] lastPathComponent];
    }

    return [NSDictionary dictionaryWithObjectsAndKeys:
            title, @"title",
            [NSNumber numberWithInt:track_num], @"track",
            nil];
}

@end
