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
#import "PathWatcher.h"

@implementation FileTreeDataSource

+ (void)initialize
{
	NSMutableDictionary *userDefaultsValuesDict = [NSMutableDictionary dictionary];
	
	[userDefaultsValuesDict setObject:[[NSURL fileURLWithPath:[@"~/Music" stringByExpandingTildeInPath]] absoluteString] forKey:@"fileTreeRootURL"];

	[[NSUserDefaults standardUserDefaults] registerDefaults:userDefaultsValuesDict];
}

- (void)awakeFromNib
{
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.fileTreeRootURL" options:0 context:nil];
	
	[self setRootURL: [NSURL URLWithString:[[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"fileTreeRootURL"]]]; 
}

- (void) observeValueForKeyPath:(NSString *)keyPath
					   ofObject:(id)object
						 change:(NSDictionary *)change
                        context:(void *)context
{
	if ([keyPath isEqualToString:@"values.fileTreeRootURL"]) {
		[self setRootURL:[NSURL URLWithString:[[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"fileTreeRootURL"]]];
	}
}


- (NSURL *)rootURL
{
	return [rootNode url];
}

- (void)setRootURL: (NSURL *)rootURL
{
	[rootNode release];
	rootNode = [[DirectoryNode alloc] initWithDataSource:self url:rootURL];

	[watcher setPath:[rootURL path]];

	[self reloadPathNode:rootNode];
}

- (PathNode *)nodeForPath:(NSString *)path
{
	NSString *relativePath = [[path stringByReplacingOccurrencesOfString:[[[self rootURL] path] stringByAppendingString:@"/"] 
															  withString:@""
															     options:NSAnchoredSearch 
																   range:NSMakeRange(0, [path length])
							   ] stringByStandardizingPath];
	PathNode *node = rootNode;
	NSLog(@"Root | Relative | Path: %@ | %@ | %@",[[self rootURL] path], relativePath, path);
	for(NSString *c in [relativePath pathComponents])
	{
		NSLog(@"COMPONENT: %@", c);
		BOOL found = NO;
		for (PathNode *subnode in [node subpaths]) {
			if ([[[[subnode url] path] lastPathComponent] isEqualToString:c]) {
				node = subnode;
				found = YES;
			}
		}
		
		if (!found)
		{
			NSLog(@"Not found!");
			return nil;
		}
	}
	
	return node;
}

- (void)pathDidChange:(NSString *)path
{
	NSLog(@"PATH DID CHANGE: %@", path);
	//Need to find the corresponding node...and call [node reloadPath], then [self reloadPathNode:node]
	PathNode *node = [self nodeForPath:path];
	NSLog(@"NODE IS: %@", node);
	[node updatePath];
	[self reloadPathNode:node];
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
    [pboard declareTypes:[NSArray arrayWithObjects:CogUrlsPboardType,nil] owner:nil];	//add it to pboard
	[pboard setData:[NSArchiver archivedDataWithRootObject:urls] forType:CogUrlsPboardType];

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
