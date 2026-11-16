//
//  ArchiveContainer.h
//  ArchiveSource
//
//  Created by Christopher Snowhill on 10/4/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "Plugin.h"

@interface ArchiveContainer : NSObject <CogContainer> {
}

// The unpack:// URL of `file` in `archive`, as ArchiveSource opens it.
+ (NSURL *)urlForFile:(NSString *)file inArchive:(NSString *)archive;

@end
