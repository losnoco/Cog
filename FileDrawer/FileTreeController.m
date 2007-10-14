//
//  FileTreeController.m
//  Cog
//
//  Created by Vincent Spader on 8/20/2006.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "FileTreeController.h"
#import "FileTreeWatcher.h"
#import "DirectoryNode.h"
#import "ImageTextCell.h"
#import "KFTypeSelectTableView.h"
#import "PlaylistLoader.h"

@implementation FileTreeController

- (void)awakeFromNib
{
	watcher = [[FileTreeWatcher alloc] init];
	[watcher setDelegate:self];
	
	[self setRootPath: [[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"fileDrawerRootPath"] ]; 
}

- (void)dealloc
{
	[rootPath release];
	[watcher release];
	
	[super dealloc];
}

- (id)rootPath
{
	return rootPath;
}

- (void)setRootPath:(id)r
{
	[r retain];
	[rootPath release];
	rootPath = r;

	[self refreshRoot];
}

- (void) refreshRoot
{
	DirectoryNode *base = [[DirectoryNode alloc] initWithPath:rootPath controller:self];
//	[self setContent: [base subpaths]];

	[base release];
	
	[watcher addPath:rootPath];
}

//BUG IN NSTREECONTROLLER'S SETCONTENT. FIX YOUR SHIT, APPLE!
- (void)setContent:(id)content
{
	if(![content isEqual:[self content]])
	{
		NSArray *paths = [[self selectionIndexPaths] retain];
		[super setContent:nil];
		[super setContent:content];
		[self setSelectionIndexPaths:paths];
		[paths release];
	}
}

- (void)refreshPath:(NSString *)path
{
	if ([path compare:rootPath] == NSOrderedSame) {
		[self refreshRoot];
		
		return;
	}
	
	NSArray *pathComponents = [path pathComponents];
	NSArray *rootComponents = [rootPath pathComponents];
	int i = 0;
	while (i < [rootComponents count] && i < [pathComponents count] &&
		   NSOrderedSame == [[rootComponents objectAtIndex: i] compare:[pathComponents objectAtIndex: i]])
	{
		i++;
	}
	

	id p;
	NSEnumerator *e = [[self content] objectEnumerator];
	while ((p = [e nextObject]))
	{
		id c = [pathComponents objectAtIndex:i];
		if (NSOrderedSame == [[[p path] lastPathComponent] compare:c]) {
			if (i == [pathComponents count] - 1) {
				[p setSubpaths:nil];
//				[self rearrangeObjects];
			}
			else {
				e = [[c subpaths] objectEnumerator];
				i++;
			}
		}
	}
}

- (NSArray *)acceptableFileTypes
{
	return [playlistLoader acceptableFileTypes];
}

- (FileTreeWatcher *)watcher
{
	return watcher;
}

- (BOOL)outlineView:(NSOutlineView *)olv writeItems:(NSArray*)items toPasteboard:(NSPasteboard*)pboard {
	//Get selected paths
	NSMutableArray *paths = [NSMutableArray arrayWithCapacity:[items count]];
	NSEnumerator *e = [items objectEnumerator];
	id p;

	while (p = [e nextObject]) {
		int i;
		id n = nil;
		NSIndexPath *ip = [p indexPath];

		for (i = 0; i < [ip length]; i++)
		{
			NSArray *a = (n == nil) ? [self content] : [n subpaths];
			n = [a objectAtIndex:[ip indexAtPosition:i]];
		}

		[paths addObject:[n path]];
	}
	
    [pboard declareTypes:[NSArray arrayWithObjects:NSFilenamesPboardType,nil] owner:nil];	//add it to pboard
	[pboard setPropertyList:paths forType:NSFilenamesPboardType];

    return YES;
}


// Required Protocol Bullshit (RPB) This is neccessary so it can be used as a datasource for drag/drop things.

 - (id)outlineView:(NSOutlineView *)outlineView child:(int)index ofItem:(id)item
 {
	 return nil;
 }
 
 - (BOOL)outlineView:(NSOutlineView *)outlineView isItemExpandable:(id)item
 {
	 return NO;
 }
 
 - (int)outlineView:(NSOutlineView *)outlineView numberOfChildrenOfItem:(id)item
 {
	 return 0;
 }
 - (id)outlineView:(NSOutlineView *)outlineView objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item
 {
	 return nil;
 }

//End of RPB


//For type-select

- (void)configureTypeSelectTableView:(KFTypeSelectTableView *)tableView
{
    [tableView setSearchWraps:YES];
}

- (int)typeSelectTableViewInitialSearchRow:(id)tableView
{
	return [tableView selectedRow];
}

// Return the string value used for type selection
- (NSString *)typeSelectTableView:(KFTypeSelectTableView *)tableView stringValueForTableColumn:(NSTableColumn *)col row:(int)row
{
	id item = [tableView itemAtRow:row];
	
	//Reaching down into NSTreeController...yikes
	return  [[[item observedObject] path] lastPathComponent];
}

//End type-select

- (void)addSelectedToPlaylist {
	NSMutableArray *urls = [[NSMutableArray alloc] init];
	NSArray *nodes = [self selectedObjects];
	NSEnumerator *e = [nodes objectEnumerator];
	
	id n;
	while (n = [e nextObject]) {
		NSURL *url = [[NSURL alloc] initFileURLWithPath:[n path]];
		[urls addObject:url];
		[url release];
	}
	
	[playlistLoader addURLs:urls sort:YES];
	[urls release];
}


@end
