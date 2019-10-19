//
//  VGMPropertiesReader.m
//  VGMStream
//
//  Created by Christopher Snowhill on 10/18/19.
//  Copyright 2019 __LoSnoCo__. All rights reserved.
//

#import "VGMPropertiesReader.h"
#import "VGMDecoder.h"
#import "VGMInterface.h"

@implementation VGMPropertiesReader

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

+ (NSDictionary *)propertiesForSource:(id<CogSource>)source
{
    VGMInfoCache * sharedMyCache = [VGMInfoCache sharedCache];

    NSURL * url = [source url];
    
    NSDictionary * properties = [sharedMyCache getPropertiesForURL:url];
    
    if (!properties) {
        int track_num = [[url fragment] intValue];
        
        NSString * path = [url absoluteString];
        NSRange fragmentRange = [path rangeOfString:@"#" options:NSBackwardsSearch];
        if (fragmentRange.location != NSNotFound) {
            path = [path substringToIndex:fragmentRange.location];
        }
        
        VGMSTREAM * stream = init_vgmstream_from_cogfile([[path stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding] UTF8String], track_num);
        if ( !stream )
            return nil;
        
        [sharedMyCache stuffURL:url stream:stream];
        
        close_vgmstream(stream);
        
        properties = [sharedMyCache getPropertiesForURL:url];
    }
    
    return properties;
}

@end
