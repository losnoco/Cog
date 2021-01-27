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
    VGMInfoCache * sharedMyCache = [VGMInfoCache sharedCache];
    
    NSDictionary * metadata = [sharedMyCache getMetadataForURL:url];
    
    if (!metadata) {
        int track_num = [[url fragment] intValue];
        
        NSString * path = [url absoluteString];
        NSRange fragmentRange = [path rangeOfString:@"#" options:NSBackwardsSearch];
        if (fragmentRange.location != NSNotFound) {
            path = [path substringToIndex:fragmentRange.location];
        }
        
        VGMSTREAM * stream = init_vgmstream_from_cogfile([[path stringByRemovingPercentEncoding] UTF8String], track_num);
        if ( !stream )
            return nil;
        
        [sharedMyCache stuffURL:url stream:stream];
        
        close_vgmstream(stream);
        
        metadata = [sharedMyCache getMetadataForURL:url];
    }
    
    return metadata;
}

@end
