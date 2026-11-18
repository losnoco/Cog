//
//  TagLibMetadataWriter.h
//  TagLib
//
//  Created by Safari on 08/11/17.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "Plugin.h"

#ifdef __cplusplus
namespace TagLib {
class File;
}

// Whether the file carries chapters TagLib can read (ID3v2 CHAP frames or
// Matroska chapters).
bool TagLibFileHasChapters(TagLib::File *file);
#endif

@interface TagLibMetadataWriter : NSObject <CogMetadataWriter> {
}

@end
