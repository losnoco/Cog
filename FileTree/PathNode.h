//
//  Node.h
//  Cog
//
//  Created by Vincent Spader on 8/20/2006.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class FileTreeDataSource;

@interface PathNode : NSObject {
	FileTreeDataSource *dataSource;

	NSURL *url;
	NSString *display; // The pretty path to display.

	NSString *lastPathComponent;

	NSImage *icon;

	NSArray *subpaths;

	NSMutableDictionary *subpathsLookup;
}

- (id)initWithDataSource:(FileTreeDataSource *)ds url:(NSURL *)u;

- (NSURL *)URL;
- (void)setURL:(NSURL *)url;

- (void)processPaths:(NSArray *)contents;

- (NSArray *)subpaths;
- (void)setSubpaths:(NSArray *)s;

- (NSString *)display;
- (void)setDisplay:(NSString *)s;

- (NSString *)lastPathComponent;
- (void)setLastPathComponent:(NSString *)s;

- (NSDictionary *)subpathsLookup;
- (void)setSubpathsLookup:(NSMutableDictionary *)d;

- (NSImage *)icon;

- (BOOL)isLeaf;

- (void)updatePath;

@end
