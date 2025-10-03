//
//  Node.m
//  Cog
//
//  Created by Vincent Spader on 8/20/2006.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "PathNode.h"

#import "CogAudio/AudioPlayer.h"

#import "FileTreeDataSource.h"

#import "ContainerNode.h"
#import "DirectoryNode.h"
#import "FileNode.h"
#import "SmartFolderNode.h"

#import "Logging.h"

@implementation PathNode

// From http://developer.apple.com/documentation/Cocoa/Conceptual/LowLevelFileMgmt/Tasks/ResolvingAliases.html
// Updated 2018-06-28
NSURL *resolveAliases(NSURL *url) {
	CFErrorRef error;
	CFDataRef bookmarkRef = CFURLCreateBookmarkDataFromFile(kCFAllocatorDefault, (__bridge CFURLRef)url, &error);
	if(bookmarkRef) {
		Boolean isStale;
		CFURLRef urlRef = CFURLCreateByResolvingBookmarkData(kCFAllocatorDefault, bookmarkRef, kCFURLBookmarkResolutionWithSecurityScope, NULL, NULL, &isStale, &error);
		CFRelease(bookmarkRef);

		if(urlRef) {
			if(!isStale) {
				return (NSURL *)CFBridgingRelease(urlRef);
			} else {
				CFRelease(urlRef);
			}
		}
	}

	// DLog(@"Not resolved");
	return url;
}

- (id)initWithDataSource:(FileTreeDataSource *)ds url:(NSURL *)u {
	self = [super init];

	if(self) {
		dataSource = ds;
		[self setURL:u];
	}

	return self;
}

- (void)setURL:(NSURL *)u {
	url = u;

	display = [[NSFileManager defaultManager] displayNameAtPath:[u path]];

	lastPathComponent = [[u path] lastPathComponent];

	icon = [[NSWorkspace sharedWorkspace] iconForFile:[url path]];

	[icon setSize:NSMakeSize(16.0, 16.0)];
}

- (NSURL *)URL {
	return url;
}

- (void)updatePath {
}

- (void)processPaths:(NSArray *)contents {
	NSMutableArray *newSubpathsDirs = [NSMutableArray new];
	NSMutableArray *newSubpaths = [NSMutableArray new];

	for(NSString *s in contents) {
		if([s characterAtIndex:0] == '.') {
			continue;
		}

		NSURL *u = [NSURL fileURLWithPath:s];
		NSString *displayName = [[NSFileManager defaultManager] displayNameAtPath:[u path]];

		PathNode *newNode;

		// DLog(@"Before: %@", u);
		u = resolveAliases(u);
		// DLog(@"After: %@", u);

		BOOL isDir;

		if([[s pathExtension] caseInsensitiveCompare:@"savedSearch"] == NSOrderedSame) {
			DLog(@"Smart folder!");
			newNode = [[SmartFolderNode alloc] initWithDataSource:dataSource url:u];
			isDir = NO;
		} else {
			[[NSFileManager defaultManager] fileExistsAtPath:[u path] isDirectory:&isDir];

			if(!isDir && ![[AudioPlayer fileTypes] containsObject:[[u pathExtension] lowercaseString]]) {
				continue;
			}

			if(isDir) {
				newNode = [[DirectoryNode alloc] initWithDataSource:dataSource url:u];
			} else if([[AudioPlayer containerTypes] containsObject:[[u pathExtension] lowercaseString]]) {
				newNode = [[ContainerNode alloc] initWithDataSource:dataSource url:u];
			} else {
				newNode = [[FileNode alloc] initWithDataSource:dataSource url:u];
			}
		}

		[newNode setDisplay:displayName];

		if(isDir)
			[newSubpathsDirs addObject:newNode];
		else
			[newSubpaths addObject:newNode];
	}

	[newSubpathsDirs addObjectsFromArray:newSubpaths];
	[self setSubpaths:newSubpathsDirs];
}

- (NSArray *)subpaths {
	if(subpaths == nil) {
		[self updatePath];
	}

	return subpaths;
}

- (void)setSubpaths:(NSArray *)s {
	subpaths = s;

	subpathsLookup = [NSMutableDictionary new];
	for(PathNode *node in s) {
		[subpathsLookup setObject:node forKey:node.lastPathComponent];
	}
}

- (NSDictionary *)subpathsLookup {
	return subpathsLookup;
}

- (void)setSubpathsLookup:(NSMutableDictionary *)d {
	subpathsLookup = d;
}

- (BOOL)isLeaf {
	return YES;
}

- (void)setDisplay:(NSString *)s {
	display = s;
}

- (NSString *)display {
	return display;
}

- (NSImage *)icon {
	return icon;
}

- (NSString *)lastPathComponent {
	return lastPathComponent;
}

- (void)setLastPathComponent:(NSString *)s {
	lastPathComponent = s;
}

@end
