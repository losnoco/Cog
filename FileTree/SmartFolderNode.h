//
//  SmartFolderNode.h
//  Cog
//
//  Created by Vincent Spader on 9/25/06.
//  Copyright 2006 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "PathNode.h"

@interface SmartFolderNode : PathNode {
    MDQueryRef _query;
}

@end
