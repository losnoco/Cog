//
//  libvgmContainer.mm
//  libvgmPlayer
//
//  Created by Christopher Snowhill on 1/02/22.
//  Copyright 2022 __LoSnoCo__. All rights reserved.
//

#import "libvgmContainer.h"
#import "libvgmDecoder.h"

#import "Logging.h"

@implementation libvgmContainer

+ (NSArray *)fileTypes
{
	return [NSArray arrayWithObjects:@"s98", @"dro", @"gym",
        @"vgm",@"vgz", // These are included so they can override AdPlug
            nil];
}

+ (NSArray *)mimeTypes 
{
	return nil;
}

+ (float)priority
{
    return [libvgmDecoder priority];
}

//This really should be source...
+ (NSArray *)urlsForContainerURL:(NSURL *)url
{
    if ([url fragment]) {
        // input url already has fragment defined - no need to expand further
        return [NSMutableArray arrayWithObject:url];
    }

    // None of the covered formats include subsongs, but dodge VGMStream and AdPlug
    return [NSArray arrayWithObject:[NSURL URLWithString:[[url absoluteString] stringByAppendingString:@"#0"]]];
}

@end
