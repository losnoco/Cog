//
//  OMPTVersionCheck.m
//  OpenMPT
//
//  Created by Christopher Snowhill on 12/27/21.
//  Copyright © 2021-2022 Christopher Snowhill. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "OMPTVersionCheck.h"

@implementation OMPTVersionCheck
+ (BOOL)shouldLoadForOSVersion:(NSOperatingSystemVersion)version {
    if (version.majorVersion < 10 ||
        (version.majorVersion == 10 && version.minorVersion < 15))
        return NO;
    else
        return YES;
}
@end
