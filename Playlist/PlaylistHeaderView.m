//
//  PlaylistHeaderView.m
//  Cog
//
//  Created by Simon on 06-05-24.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "PlaylistHeaderView.h"


@implementation PlaylistHeaderView

- (void)mouseDown:(NSEvent *)theEvent
{
	NSPoint event_location = [theEvent locationInWindow];
	NSPoint local_point = [self convertPoint:event_location fromView:nil];
	
	int column = [self columnAtPoint:local_point];

	if ([theEvent clickCount] == 2 && column != -1) {

		// compute whether the clickpoint is a column separator or not
		BOOL clickedSeperator = NO;
		// handle a click one pixel away at right
		NSRect rect = [self headerRectOfColumn:column];
		if (abs(rect.origin.x - local_point.x) <= 1 && column > 0) {
			--column;
			clickedSeperator = YES;
		}
		// handle a click 3 pixels away at left
		else if (abs(rect.origin.x + rect.size.width - local_point.x) <= 3)
			clickedSeperator = YES;
		
		if (clickedSeperator) {
			NSTableColumn *col = [[[self tableView] tableColumns] objectAtIndex:column];
			
			//Info about the font and such
			NSCell *cell = [col dataCell];

			//Binding info...reaching deep!
			NSDictionary *bindingInfo = [col infoForBinding:@"value"];
			NSArray *boundArray = [[bindingInfo objectForKey:NSObservedObjectKey] valueForKeyPath:[bindingInfo objectForKey:NSObservedKeyPathKey]];
			
			//find the longest string display length in that column
			float max_width = -1;
			for (id row in boundArray) {
			
				[cell setObjectValue:row];
				
				float width = [cell cellSize].width;
				if (width > max_width)
					max_width = width;
			}
			

			// set the new width (plus a 5 pixel extra to avoid "..." string substitution)
			[col setWidth:max_width+5];
				
		}
		else
			[super mouseDown: theEvent];
	}
	else
		[super mouseDown: theEvent];
}

@end
