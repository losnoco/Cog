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

+ (void)initialize
{
    register_log_callback();
}

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
    NSString * path = [[url absoluteString] stringByRemovingPercentEncoding];

    if ([url fragment]) {
        // input url might be a TXTP with fragment parameters, in which case
        // we need to parse that anyway
        NSString * frag = [url fragment];
        NSUInteger len = [frag length];
        if (len < 5 || ![[frag substringFromIndex:len - 5] isEqualToString:@".txtp"]) {
            // input url already has fragment defined - no need to expand further
            return [NSMutableArray arrayWithObject:url];
        }
        
        path = [path stringByAppendingFormat:@"%%23%@", frag];
        url = [NSURL fileURLWithPath:[path stringByRemovingPercentEncoding]];
    }
    
    VGMSTREAM * stream = init_vgmstream_from_cogfile([path UTF8String], 0);
	if (!stream)
	{
		ALog(@"Open failed for file: %@", [url absoluteString]);
		return [NSArray array];
	}

    VGMInfoCache * sharedMyCache = [VGMInfoCache sharedCache];

    NSMutableArray *tracks = [NSMutableArray array];
    NSURL * trackurl;
    
	int i;
    int subsongs = stream->num_streams;
    if (subsongs == 0)
        subsongs = 1;

    {
        trackurl = [NSURL URLWithString:[[url absoluteString] stringByAppendingString:@"#1"]];
        [sharedMyCache stuffURL:trackurl stream:stream];
        [tracks addObject:trackurl];
    }
    
    for (i = 2; i <= subsongs; ++i) {
        close_vgmstream(stream);
        
        stream = init_vgmstream_from_cogfile([path UTF8String], i);
        
        if (!stream)
            return [NSArray array];
        
        trackurl = [NSURL URLWithString:[[url absoluteString] stringByAppendingFormat:@"#%i", i]];
        [sharedMyCache stuffURL:trackurl stream:stream];
        [tracks addObject:trackurl];
    }
    
    close_vgmstream(stream);
    
	return tracks;
}


@end
