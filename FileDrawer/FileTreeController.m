//
//  FileTreeController.m
//  Cog
//
//  Created by Vincent Spader on 8/20/2006.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "FileTreeController.h"
#import "DirectoryNode.h"
#import "ImageTextCell.h"
#import "KFTypeSelectTableView.h"

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
	
	NSLog(@"DEALLOCATING CONTROLLER");
	
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

	[watcher removePath:r];
	[watcher addPath:rootPath];
	
	[self refreshRoot];
}

- (void) refreshRoot
{
	DirectoryNode *base = [[DirectoryNode alloc] initWithPath:rootPath controller:self];
	NSLog(@"Subpaths: %i", [[base subpaths] count]);
	[self setContent: [base subpaths]];
	NSLog(@"Test: %i", [[self content] retainCount]);

	[base release];
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
	return [playlistController acceptableFileTypes];
}

- (FileTreeWatcher *)watcher
{
	return watcher;
}

- (BOOL)outlineView:(NSOutlineView *)olv writeItems:(NSArray*)items toPasteboard:(NSPasteboard*)pboard {
	//Get selected paths
	NSLog(@"Items: %@", items);
	NSMutableArray *paths = [NSMutableArray arrayWithCapacity:[items count]];
	id p;
	NSEnumerator *e = [items objectEnumerator];

	while (p = [e nextObject]) {
		int i;
		PathNode *n = nil;
		NSIndexPath *ip = [p indexPath];
		NSLog(@"Content: %@", n);
		for (i = 0; i < [ip length]; i++)
		{
			NSArray *a = (n == nil) ? [self content] : [n subpaths];
			n = [a objectAtIndex:[ip indexAtPosition:i]];
		}
		NSLog(@"Path: %@", n);
		[paths addObject:[n path]];
	}
	
    [pboard declareTypes:[NSArray arrayWithObjects:NSFilenamesPboardType,nil] owner:nil];	//add it to pboard
	[pboard setPropertyList:paths forType:NSFilenamesPboardType];

    return YES;
}

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

@end
