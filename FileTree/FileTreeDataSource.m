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

#import "Logging.h"

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

	[pathControl setTarget:self];
    [pathControl setAction:@selector(pathControlAction:)];
}

- (void) observeValueForKeyPath:(NSString *)keyPath
					   ofObject:(id)object
						 change:(NSDictionary *)change
                        context:(void *)context
{
	DLog(@"File tree root URL: %@\n", [[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"fileTreeRootURL"]);
	if ([keyPath isEqualToString:@"values.fileTreeRootURL"]) {
		[self setRootURL:[NSURL URLWithString:[[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"fileTreeRootURL"]]];
	}
}

- (void)changeURL:(NSURL *)url
{
	if (url != nil)
	{
		[[[NSUserDefaultsController sharedUserDefaultsController] defaults] setObject:[url absoluteString] forKey:@"fileTreeRootURL"];
	}
}

- (void)pathControlAction:(id)sender
{
	if ([pathControl clickedPathComponentCell] != nil && [[pathControl clickedPathComponentCell] URL] != nil)
	{
		[self changeURL:[[pathControl clickedPathComponentCell] URL]];
	}
}

- (NSURL *)rootURL
{
	return [rootNode URL];
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
	DLog(@"Root | Relative | Path: %@ | %@ | %@",[[self rootURL] path], relativePath, path);
	for (NSString *c in [relativePath pathComponents])
	{
		DLog(@"COMPONENT: %@", c);
		BOOL found = NO;
		for (PathNode *subnode in [node subpaths]) {
			if ([[[[subnode URL] path] lastPathComponent] isEqualToString:c]) {
				node = subnode;
				found = YES;
			}
		}
		
		if (!found)
		{
			DLog(@"Not found!");
			return nil;
		}
	}
	
	return node;
}

- (void)pathDidChange:(NSString *)path
{
	DLog(@"PATH DID CHANGE: %@", path);
	//Need to find the corresponding node...and call [node reloadPath], then [self reloadPathNode:node]
	PathNode *node = [self nodeForPath:path];
	DLog(@"NODE IS: %@", node);
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
	NSMutableArray *paths = [NSMutableArray arrayWithCapacity:[items count]];

	for (id p in items) {
		[urls addObject:[p URL]];
		[paths addObject:[[p URL] path]];
	}
	DLog(@"Paths: %@", paths);
	[pboard declareTypes:[NSArray arrayWithObjects:CogUrlsPboardType,nil] owner:nil];	//add it to pboard
	[pboard setData:[NSArchiver archivedDataWithRootObject:urls] forType:CogUrlsPboardType];
	[pboard addTypes:[NSArray arrayWithObject:NSFilenamesPboardType] owner:self];
	[pboard setPropertyList:paths forType:NSFilenamesPboardType];

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
