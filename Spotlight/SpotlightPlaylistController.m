//
//  SpotlightPlaylistController.m
//  Cog
//
//  Created by Matthew Grinshpun on 13/02/08.
//  Copyright 2008 Matthew Leon Grinshpun. All rights reserved.
//

#import "SpotlightPlaylistController.h"
#import "SpotlightWindowController.h"

#import "PlaylistEntry.h"

static NSArray *cellIdentifiers = nil;

@implementation SpotlightPlaylistController

+ (void)initialize {
	cellIdentifiers = @[@"title", @"artist", @"album", @"length", @"year", @"genre", @"track"];
}

// Allow drag and drop from Spotlight into main playlist
- (BOOL)tableView:(NSTableView *)tv
writeRowsWithIndexes:(NSIndexSet *)rowIndexes
        toPasteboard:(NSPasteboard *)pboard {
	[spotlightWindowController.query disableUpdates];

	NSArray *urls = [[self selectedObjects] valueForKey:@"URL"];
	NSError *error = nil;
	NSData *data = [NSKeyedArchiver archivedDataWithRootObject:urls
	                                     requiringSecureCoding:YES
	                                                     error:&error];
	if(error) return NO;
	[pboard declareTypes:@[CogUrlsPboardType] owner:nil]; // add it to pboard
	[pboard setData:data forType:CogUrlsPboardType];

	[spotlightWindowController.query enableUpdates];

	return YES;
}

// Do not accept drag operations, necessary as long as this class inherits from PlaylistController
- (NSDragOperation)tableView:(NSTableView *)tv
                validateDrop:(id<NSDraggingInfo>)info
                 proposedRow:(NSInteger)row
       proposedDropOperation:(NSTableViewDropOperation)op {
	return NSDragOperationNone;
}

- (NSView *_Nullable)tableView:(NSTableView *)tableView viewForTableColumn:(NSTableColumn *_Nullable)tableColumn row:(NSInteger)row {
	NSString *cellText = @"";
	NSString *cellIdentifier = @"";
	NSTextAlignment cellTextAlignment = NSTextAlignmentLeft;

	PlaylistEntry *pe = [[self arrangedObjects] objectAtIndex:row];

	if(pe) {
		cellIdentifier = [tableColumn identifier];
		NSUInteger index = [cellIdentifiers indexOfObject:cellIdentifier];

		switch(index) {
			case 0:
				if([pe title]) cellText = pe.title;
				break;

			case 1:
				if([pe artist]) cellText = pe.artist;
				break;

			case 2:
				if([pe album]) cellText = pe.album;
				break;

			case 3:
				cellText = pe.lengthText;
				cellTextAlignment = NSTextAlignmentRight;
				break;

			case 4:
				if([pe year]) cellText = pe.yearText;
				cellTextAlignment = NSTextAlignmentRight;
				break;

			case 5:
				if([pe genre]) cellText = pe.genre;
				break;

			case 6:
				if([pe track]) cellText = pe.trackText;
				cellTextAlignment = NSTextAlignmentRight;
				break;
		}
	}

	NSString *cellTextTruncated = cellText;
	if([cellTextTruncated length] > 1023) {
		cellTextTruncated = [cellTextTruncated substringToIndex:1023];
		cellTextTruncated = [cellTextTruncated stringByAppendingString:@"…"];
	}

	NSView *view = [tableView makeViewWithIdentifier:cellIdentifier owner:nil];
	if(view && [view isKindOfClass:[NSTableCellView class]]) {
		NSTableCellView *cellView = (NSTableCellView *)view;
		NSRect frameRect = cellView.frame;
		frameRect.origin.y = 1;
		frameRect.size.height = tableView.rowHeight;
		cellView.frame = frameRect;

		if(cellView.textField) {
			cellView.textField.allowsDefaultTighteningForTruncation = YES;

			NSFont *font = [NSFont monospacedDigitSystemFontOfSize:13 weight:NSFontWeightRegular];

			cellView.textField.font = font;
			cellView.textField.stringValue = cellTextTruncated;
			cellView.textField.alignment = cellTextAlignment;

			if(cellView.textField.intrinsicContentSize.width > cellView.textField.frame.size.width - 4)
				cellView.textField.toolTip = cellTextTruncated;
			else
				cellView.textField.toolTip = [pe statusMessage];

			NSRect cellFrameRect = cellView.textField.frame;
			cellFrameRect.origin.y = 1;
			cellFrameRect.size.height = frameRect.size.height;
			cellView.textField.frame = cellFrameRect;
		}

		cellView.rowSizeStyle = NSTableViewRowSizeStyleCustom;
	}

	return view;
}

@end
