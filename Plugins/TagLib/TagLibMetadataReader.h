//
//  TagLibMetadataReader.h
//  TagLib
//
//  Created by Vincent Spader on 2/24/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "Plugin.h"

namespace TagLib {
class Tag;
}

@interface TagLibMetadataReader : NSObject <CogMetadataReader> {
}

+ (NSDictionary *)readMetadataFromTag:(const TagLib::Tag *)tag;

@end
