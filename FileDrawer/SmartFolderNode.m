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
	NSEnumerator *e = [contents objectEnumerator];
	NSString *s;
	
	while (s = [e nextObject])
	{
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
			
			if (!isDir && ![[controller acceptableFileTypes] containsObject:[s pathExtension]])
				continue;

			if (isDir)
				newNode = [[DirectoryNode alloc] initWithPath: s controller:controller];
			else
				newNode = [[FileNode alloc] initWithPath: s];
		}
					
		[subpaths addObject:newNode];

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
		
		// Ugh, Carbon from now on...
		MDQueryRef query = MDQueryCreate(kCFAllocatorDefault, (CFStringRef)rawQuery, NULL, NULL);
		
		MDQuerySetSearchScope(query, (CFArrayRef)searchPaths, 0);
		
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(queryFinished:) name:(NSString*)kMDQueryDidFinishNotification object:(id)query];
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(queryUpdate:) name:(NSString*)kMDQueryDidUpdateNotification object:(id)query];

		MDQueryExecute(query, kMDQueryWantsUpdates);
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

	MDQueryDisableUpdates(query);
	int c = MDQueryGetResultCount(query);
	
	int i;
	for (i = 0; i < c; i++)
	{
		MDItemRef  item = (MDItemRef)MDQueryGetResultAtIndex(query, i);
		
		NSString *itemPath = (NSString*)MDItemCopyAttribute(item, kMDItemPath);
		
		[results addObject:itemPath];
		
		[itemPath release];
	}

	MDQueryEnableUpdates(query);
	
	[self processContents:results];
	[self setSubpaths:subpaths];
}

- (void)queryUpdate:(NSNotification *)notification
{
	[subpaths removeAllObjects];
	[self queryFinished: notification];
}


@end

