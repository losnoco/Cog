//
//  Node.h
//  Cog
//
//  Created by Vincent Spader on 8/20/2006.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface PathNode : NSObject
{
	NSString *path;
	NSImage *icon;

	NSArray *subpaths;
}

- (id)initWithPath:(NSString *)p;

- (NSString *)path;
- (void)setPath:(NSString *)p;

- (void)processPaths: (NSArray *)contents;

- (NSArray *)subpaths;
- (void)setSubpaths:(NSArray *)s;

- (NSImage *)icon;

- (BOOL)isLeaf;

@end
