
#import "DNDArrayController.h"

#import "Logging.h"

@implementation DNDArrayController

NSString *MovedRowsType = @"MOVED_ROWS_TYPE";
NSString *CogUrlsPboardType = @"COG_URLS_TYPE";

// @"CorePasteboardFlavorType 0x6974756E" is the "itun" type representing an iTunes plist
NSString *iTunesDropType = @"CorePasteboardFlavorType 0x6974756E";

- (void)awakeFromNib
{
    // register for drag and drop
    [tableView registerForDraggedTypes:[NSArray arrayWithObjects:MovedRowsType, CogUrlsPboardType, NSFilenamesPboardType, iTunesDropType, nil]];
}


- (BOOL)tableView:(NSTableView *)aTableView writeRowsWithIndexes:(NSIndexSet *)rowIndexes toPasteboard:(NSPasteboard *)pboard
{
	DLog(@"INDEX SET ON DRAG: %@", rowIndexes);
			
	NSData *data = [NSArchiver archivedDataWithRootObject:rowIndexes];
	
	[pboard declareTypes: [NSArray arrayWithObjects:MovedRowsType, nil] owner:self];
	[pboard setData:data forType: MovedRowsType];

	return YES;
}


- (NSDragOperation)tableView:(NSTableView*)tv
				validateDrop:(id <NSDraggingInfo>)info
				 proposedRow:(int)row
	   proposedDropOperation:(NSTableViewDropOperation)op
{
	NSDragOperation dragOp = NSDragOperationCopy;
	
    if ([info draggingSource] == tv)
		dragOp = NSDragOperationMove;
	
	DLog(@"VALIDATING DROP!");
    // we want to put the object at, not over,
    // the current row (contrast NSTableViewDropOn) 
    [tv setDropRow:row dropOperation:NSTableViewDropAbove];
	
    return dragOp;
}


- (BOOL)tableView:(NSTableView*)tv
	   acceptDrop:(id <NSDraggingInfo>)info
			  row:(int)row
	dropOperation:(NSTableViewDropOperation)op
{
    if (row < 0)
	{
		row = 0;
	}

    // if drag source is self, it's a move
    if ([info draggingSource] == tableView)
	{
		NSIndexSet *indexSet = [NSUnarchiver unarchiveObjectWithData:[[info draggingPasteboard] dataForType:MovedRowsType]];
		if (indexSet) 
		{
			DLog(@"INDEX SET ON DROP: %@", indexSet);
			NSArray *selected = [[self arrangedObjects] objectsAtIndexes:indexSet];
			[self moveObjectsInArrangedObjectsFromIndexes:indexSet toIndex:row];
			
			[self setSelectedObjects:selected];
			
			DLog(@"ACCEPTING DROP!");
			return YES;
		}
	}
	DLog(@"REJECTING DROP!");
	return NO;
}


-(void) moveObjectsInArrangedObjectsFromIndexes:(NSIndexSet*)indexSet
										toIndex:(unsigned int)insertIndex
{
	
    NSArray		*objects = [self arrangedObjects];
	NSUInteger  index = [indexSet lastIndex];
	
    int			aboveInsertIndexCount = 0;
    id			object;
    int			removeIndex;
	
    while (NSNotFound != index)
	{
		if (index >= insertIndex) {
			removeIndex = (int)(index + aboveInsertIndexCount);
			aboveInsertIndexCount += 1;
		}
		else
		{
			removeIndex = (int)index;
			insertIndex -= 1;
		}
		
		object = [objects objectAtIndex:removeIndex];

		[self removeObjectAtArrangedObjectIndex:removeIndex];
		[self insertObject:object atArrangedObjectIndex:insertIndex];
		
		index = [indexSet indexLessThanIndex:index];
    }
}


@end
