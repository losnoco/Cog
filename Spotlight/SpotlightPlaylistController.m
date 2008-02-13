//
//  SpotlightPlaylistController.m
//  Cog
//
//  Created by Matthew Grinshpun on 13/02/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "SpotlightPlaylistController.h"

@implementation SpotlightPlaylistController

// Allow drag and drop from Spotlight into main playlist
- (BOOL)tableView:(NSTableView *)tv
    writeRowsWithIndexes:(NSIndexSet *)rowIndexes
	        toPasteboard:(NSPasteboard*)pboard
{
    [spotlightSearchController.query disableUpdates];
    
    NSArray *urls = [[self selectedObjects]valueForKey:@"url"];
    [pboard declareTypes:[NSArray arrayWithObjects:CogUrlsPbboardType,nil] owner:nil];	//add it to pboard
	[pboard setData:[NSArchiver archivedDataWithRootObject:urls] forType:CogUrlsPbboardType];
    
    [spotlightSearchController.query enableUpdates];
	
	return YES;
}

// Do not accept drag operations, necessary as long as this class inherits from PlaylistController
- (NSDragOperation)tableView:(NSTableView*)tv
                validateDrop:(id <NSDraggingInfo>)info
                 proposedRow:(int)row
       proposedDropOperation:(NSTableViewDropOperation)op
{
    return NSDragOperationNone;
}

@end