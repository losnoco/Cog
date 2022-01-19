//
//  SpotlightPlaylistController.m
//  Cog
//
//  Created by Matthew Grinshpun on 13/02/08.
//  Copyright 2008 Matthew Leon Grinshpun. All rights reserved.
//

#import "SpotlightPlaylistController.h"
#import "SpotlightWindowController.h"

@implementation SpotlightPlaylistController

// Allow drag and drop from Spotlight into main playlist
- (BOOL)tableView:(NSTableView *)tv
    writeRowsWithIndexes:(NSIndexSet *)rowIndexes
	        toPasteboard:(NSPasteboard*)pboard
{
    [spotlightWindowController.query disableUpdates];
    
    NSArray *urls = [[self selectedObjects]valueForKey:@"URL"];
    NSError *error = nil;
    NSData *data;
    if (@available(macOS 10.13, *))
    {
        data = [NSKeyedArchiver archivedDataWithRootObject:urls
                                     requiringSecureCoding:YES
                                                     error:&error];
        if (error) return NO;
    }
    else
    {
        data = [NSArchiver archivedDataWithRootObject:urls];
    }
    [pboard declareTypes:@[CogUrlsPboardType] owner:nil];	//add it to pboard
    [pboard setData:data forType:CogUrlsPboardType];
    
    [spotlightWindowController.query enableUpdates];
	
	return YES;
}

// Do not accept drag operations, necessary as long as this class inherits from PlaylistController
- (NSDragOperation)tableView:(NSTableView*)tv
                validateDrop:(id <NSDraggingInfo>)info
                 proposedRow:(NSInteger)row
       proposedDropOperation:(NSTableViewDropOperation)op
{
    return NSDragOperationNone;
}

@end
