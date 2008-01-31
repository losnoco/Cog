//
//  FileTreeDataSource.m
//  Cog
//
//  Created by Vincent Spader on 10/14/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "FileTreeDataSource.h"

#import "DNDArrayController.h"

#import "DirectoryNode.h"

@implementation FileTreeDataSource

- (void)awakeFromNib
{
	[self setRootPath: [[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"fileDrawerRootPath"] ]; 
}

- (NSString *)rootPath
{
	return [[rootNode url] path];
}

- (void)setRootPath: (NSString *)rootPath
{
	[[[[outlineView tableColumns] objectAtIndex:0] headerCell] setStringValue:[[NSFileManager defaultManager] displayNameAtPath:rootPath]];
	
	[rootNode release];
	rootNode = [[DirectoryNode alloc] initWithDataSource:self url:[NSURL fileURLWithPath:rootPath]];

	[self reloadPathNode:rootNode];
}

- (int)outlineView:(NSOutlineView *)outlineView numberOfChildrenOfItem:(id)item
{
	PathNode *n = (item == nil ? rootNode : item);

    return [[n subpaths] count];
}

 
- (BOOL)outlineView:(NSOutlineView *)outlineView isItemExpandable:(id)item 
{
	PathNode *n = (item == nil ? rootNode : item);

    return ([n isLeaf] == NO);
}

- (id)outlineView:(NSOutlineView *)outlineView child:(int)index ofItem:(id)item
{
	PathNode *n = (item == nil ? rootNode : item);

    return [[n subpaths] objectAtIndex:index];
}

- (id)outlineView:(NSOutlineView *)outlineView objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item
{
	PathNode *n = (item == nil ? rootNode : item);

	return n;
}

//Drag it drop it
- (BOOL)outlineView:(NSOutlineView *)outlineView writeItems:(NSArray*)items toPasteboard:(NSPasteboard*)pboard {
	//Get selected paths
	NSMutableArray *urls = [NSMutableArray arrayWithCapacity:[items count]];
	NSEnumerator *e = [items objectEnumerator];
	id p;

	while (p = [e nextObject]) {
		[urls addObject:[p url]];
	}
	NSLog(@"URLS: %@", urls);
    [pboard declareTypes:[NSArray arrayWithObjects:CogUrlsPbboardType,nil] owner:nil];	//add it to pboard
	[pboard setData:[NSArchiver archivedDataWithRootObject:urls] forType:CogUrlsPbboardType];

    return YES;
}

- (void)reloadPathNode:(PathNode *)item
{
	if (item == rootNode)
	{
		[outlineView reloadData];
	}
	else
	{
		[outlineView reloadItem:item reloadChildren:YES];
	}
}

- (void)dealloc
{
	[rootNode release];
	
	[super dealloc];
}

@end
