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

@implementation SmartFolderNode

-(id)initWithPath:(NSString *)p controller:(id) c
{
	self = [super initWithPath:p];
	if (self)
	{
		controller = [c retain];
	}
	
	return self;
}

- (void)dealloc {
	if (subpaths)
		[subpaths release];
	
	[super dealloc];
}


- (BOOL)isLeaf
{
	return NO;
}

//need to merge this and directorynode
- (void)processContents: (NSArray *)contents
{
	NSLog(@"PROCESSING: %@", contents);
	NSEnumerator *e = [contents objectEnumerator];
	NSString *s;
	
	while (s = [e nextObject])
	{
		NSLog(@"STRING: %@", s);
/*		if ([s characterAtIndex:0] == '.')
		{
			continue;
		}
*/		
		PathNode *newNode;
//		NSString *newSubpath = [path stringByAppendingPathComponent: s];
	
		if ([[s pathExtension] caseInsensitiveCompare:@"savedSearch"] == NSOrderedSame)
		{
			newNode = [[SmartFolderNode alloc] initWithPath:s controller:controller];
		}
		else
		{
			BOOL isDir;
			
			[[NSFileManager defaultManager] fileExistsAtPath:s isDirectory:&isDir];
			
			NSLog(@"IS IT ACCEPTABLE?");
			if (!isDir && ![[controller acceptableFileTypes] containsObject:[s pathExtension]])
				continue;
			NSLog(@"IS IT A FILE?");
			if (isDir)
				newNode = [[DirectoryNode alloc] initWithPath: s controller:controller];
			else
				newNode = [[FileNode alloc] initWithPath: s];
		}
					
		[subpaths addObject:newNode];
		NSLog(@"SUBPATHS IN PROCESS: %@", subpaths);
		[newNode release];
	}
}

- (NSArray *)subpaths
{
	if (subpaths == nil)
	{
		subpaths = [[NSMutableArray alloc] init];
	
		NSDictionary *doc = [NSDictionary dictionaryWithContentsOfFile:path];
		NSString *rawQuery = [doc objectForKey:@"RawQuery"];
		NSArray *searchPaths = [[doc objectForKey:@"SearchCriteria"] objectForKey:@"CurrentFolderPath"];
		
		NSLog(@"Query: %@", rawQuery);
		// Ugh, Carbon from now on...
		MDQueryRef query = MDQueryCreate(kCFAllocatorDefault, (CFStringRef)rawQuery, NULL, NULL);
		
		MDQuerySetSearchScope(query, (CFArrayRef)searchPaths, 0);
		
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(queryFinished:) name:(NSString*)kMDQueryDidFinishNotification object:(id)query];
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(queryUpdate:) name:(NSString*)kMDQueryDidUpdateNotification object:(id)query];

		NSLog(@"PATHS: %@", searchPaths);
		MDQueryExecute(query, kMDQueryWantsUpdates | kMDQuerySynchronous);
		NSLog(@"QUERY FINISHED: %@", subpaths);
	}
	
	return subpaths;
}

- (void)setSubpaths:(id)s
{
	[s retain];
	[subpaths release];
	subpaths = s;
}

- (unsigned int)countOfSubpaths
{
	return [[self subpaths] count];
}

- (PathNode *)objectInSubpathsAtIndex:(unsigned int)index
{
	return [[self subpaths] objectAtIndex:index];
}

- (void)queryFinished:(NSNotification *)notification
{
	MDQueryRef query = [notification object];

	NSMutableArray *results = [NSMutableArray array];

	int c = MDQueryGetResultCount(query);
	
	int i;
	for (i = 0; i < c; i++)
	{
		MDItemRef  item = (MDItemRef)MDQueryGetResultAtIndex(query, i);
		
		NSString *itemPath = (NSString*)MDItemCopyAttribute(item, kMDItemPath);
		NSLog(@"RESULT PATH: %@", itemPath);
		
		[results addObject:itemPath];
		
		[itemPath release];
	}
	
	[self processContents:results];
	NSLog(@"CONTENTS PROCESSED");
}

- (void)queryUpdate:(NSNotification *)notification
{
	MDQueryRef query = [notification object];
	NSLog(@"QUERY UPDATE: %@", notification);
}


@end

