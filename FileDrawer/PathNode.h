//
//  Node.h
//  Cog
//
//  Created by Vincent Spader on 8/20/2006.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "PathIcon.h"

@interface PathNode : NSObject
{
	NSString *path;
	PathIcon *pathIcon;
}

- (id)initWithPath:(NSString *)p;

- (id)pathIcon;
- (void)setPathIcon:(id)pi;

@end
