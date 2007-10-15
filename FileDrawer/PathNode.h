//
//  Node.h
//  Cog
//
//  Created by Vincent Spader on 8/20/2006.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class FileTreeDataSource;

@interface PathNode : NSObject
{
	FileTreeDataSource *dataSource;
	
	NSString *path;
	NSString *displayPath; //The pretty path to display.
	
	NSImage *icon;

	NSArray *subpaths;
}

- (id)initWithDataSource:(FileTreeDataSource *)ds path:(NSString *)p;

- (NSString *)path;
- (void)setPath:(NSString *)p;

- (void)processPaths: (NSArray *)contents;

- (NSArray *)subpaths;
- (void)setSubpaths:(NSArray *)s;

- (NSString *)displayPath;
- (NSImage *)icon;

- (BOOL)isLeaf;

- (void)startWatching;
- (void)stopWatching;
- (void)updatePathNotification:(NSNotification *)notification;

@end
