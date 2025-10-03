//
//  SmartFolderNode.m
//  Cog
//
//  Created by Vincent Spader on 9/25/06.
//  Copyright 2006 __MyCompanyName__. All rights reserved.
//

#import "SmartFolderNode.h"
#import "DirectoryNode.h"
#import "FileNode.h"
#import "FileTreeDataSource.h"

#import "Logging.h"

#import "SandboxBroker.h"

@implementation SmartFolderNode

- (BOOL)isLeaf {
	return NO;
}

- (void)updatePath {
	const void *sbHandle = [[SandboxBroker sharedSandboxBroker] beginFolderAccess:url];
	NSDictionary *doc = [NSDictionary dictionaryWithContentsOfFile:[url path]];
	[[SandboxBroker sharedSandboxBroker] endFolderAccess:sbHandle];
	NSString *rawQuery = [doc objectForKey:@"RawQuery"];
	NSArray *searchPaths = [[doc objectForKey:@"SearchCriteria"] objectForKey:@"CurrentFolderPath"];

	// Ugh, Carbon from now on...
	MDQueryRef query = MDQueryCreate(kCFAllocatorDefault, (CFStringRef)rawQuery, NULL, NULL);
	_query = query;

	MDQuerySetSearchScope(query, (CFArrayRef)searchPaths, 0);

	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(queryFinished:) name:(NSString *)kMDQueryDidFinishNotification object:(__bridge id)query];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(queryUpdate:) name:(NSString *)kMDQueryDidUpdateNotification object:(__bridge id)query];

	DLog(@"Making query!");
	MDQueryExecute(query, kMDQueryWantsUpdates);

	// Note: This is asynchronous!
}

- (void)setSubpaths:(id)s {
	subpaths = s;

	subpathsLookup = [NSMutableDictionary new];
	for(PathNode *node in s) {
		[subpathsLookup setObject:node forKey:node.lastPathComponent];
	}
}

- (unsigned int)countOfSubpaths {
	return (unsigned int)[[self subpaths] count];
}

- (PathNode *)objectInSubpathsAtIndex:(unsigned int)index {
	return [[self subpaths] objectAtIndex:index];
}

- (void)queryFinished:(NSNotification *)notification {
	DLog(@"Query finished!");
	MDQueryRef query = (__bridge MDQueryRef)[notification object];

	NSMutableArray *results = [NSMutableArray array];

	MDQueryDisableUpdates(query);
	int c = (int)MDQueryGetResultCount(query);

	int i;
	for(i = 0; i < c; i++) {
		MDItemRef item = (MDItemRef)MDQueryGetResultAtIndex(query, i);

		NSString *itemPath = (NSString *)CFBridgingRelease(MDItemCopyAttribute(item, kMDItemPath));

		[results addObject:itemPath];
	}

	MDQueryEnableUpdates(query);

	DLog(@"Query update!");

	[self processPaths:[results sortedArrayUsingSelector:@selector(caseInsensitiveCompare:)]];

	[dataSource reloadPathNode:self];
}

- (void)queryUpdate:(NSNotification *)notification {
	DLog(@"Query update!");
	[self queryFinished:notification];
}

- (void)dealloc {
	if(_query) CFRelease(_query);
}

@end
