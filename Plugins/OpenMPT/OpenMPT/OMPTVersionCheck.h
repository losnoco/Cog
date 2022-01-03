//
//  OMPTVersionCheck.h
//  OpenMPT
//
//  Created by Christopher Snowhill on 12/27/21.
//  Copyright © 2021-2022 Christopher Snowhill. All rights reserved.
//

#ifndef OMPTVersionCheck_h
#define OMPTVersionCheck_h

#import "Plugin.h"

@interface OMPTVersionCheck : NSObject<CogVersionCheck>
+ (BOOL)shouldLoadForOSVersion:(NSOperatingSystemVersion)version;
@end

#endif /* OMPTVersionCheck_h */
