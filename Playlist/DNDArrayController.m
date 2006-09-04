
#import "DNDArrayController.h"

@implementation DNDArrayController

NSString *MovedRowsType = @"MOVED_ROWS_TYPE";


- (void)awakeFromNib
{
    // register for drag and drop
	NSLog(@"AWOKE");
    [tableView registerForDraggedTypes:[NSArray arrayWithObjects:MovedRowsType, NSFilenamesPboardType, nil]];
//	[tableView setVerticalMotionCanBeginDrag:YES];
//    [tableView setAllowsMultipleSelection:NO];
//	[super awakeFromNib];
//	DBLog(@"HERE: %@", [tableView registeredDraggedTypes]);
}


- (BOOL)tableView:(NSTableView *)tv
		writeRows:(NSArray*)rows
	 toPasteboard:(NSPasteboard*)pboard
{
	NSLog(@"WRITE ROWS");
	
	NSData *data;
	data = [NSKeyedArchiver archivedDataWithRootObject:rows];
	
	[pboard declareTypes: [NSArray arrayWithObjects:MovedRowsType, nil] owner:self];
	[pboard setData: data forType: MovedRowsType];

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
	NSLog(@"ACCEPTATING");
    // if drag source is self, it's a move
    if ([info draggingSource] == tableView)
	{
//		DBLog(@"ACCEPTATED");
		NSArray *rows = [NSKeyedUnarchiver unarchiveObjectWithData:[[info draggingPasteboard] dataForType: MovedRowsType]];
	
		NSIndexSet  *indexSet = [self indexSetFromRows:rows];
	
		[self moveObjectsInArrangedObjectsFromIndexes:indexSet toIndex:row];
	
		// set selected rows to those that were just moved
		// Need to work out what moved where to determine proper selection...
		int rowsAbove = [self rowsAboveRow:row inIndexSet:indexSet];
		
		NSRange range = NSMakeRange(row - rowsAbove, [indexSet count]);
		indexSet = [NSIndexSet indexSetWithIndexesInRange:range];
		[self setSelectionIndexes:indexSet];
		
		return YES;
	}
	
	return NO;
}


-(void) moveObjectsInArrangedObjectsFromIndexes:(NSIndexSet*)indexSet
										toIndex:(unsigned int)insertIndex
{
	
    NSArray		*objects = [self arrangedObjects];
	int			index = [indexSet lastIndex];
	
    int			aboveInsertIndexCount = 0;
    id			object;
    int			removeIndex;
	
    while (NSNotFound != index)
	{
		if (index >= insertIndex) {
			removeIndex = index + aboveInsertIndexCount;
			aboveInsertIndexCount += 1;
		}
		else
		{
			removeIndex = index;
			insertIndex -= 1;
		}
		object = [objects objectAtIndex:removeIndex];
		[object retain];
		[self removeObjectAtArrangedObjectIndex:removeIndex];
		[self insertObject:object atArrangedObjectIndex:insertIndex];
		[object release];
		
		index = [indexSet indexLessThanIndex:index];
    }
}


- (NSIndexSet *)indexSetFromRows:(NSArray *)rows
{
    NSMutableIndexSet *indexSet = [NSMutableIndexSet indexSet];
    NSEnumerator *rowEnumerator = [rows objectEnumerator];
    NSNumber *idx;
    while (idx = [rowEnumerator nextObject])
    {
		[indexSet addIndex:[idx unsignedIntValue]];
    }
    return indexSet;
}


- (int)rowsAboveRow:(int)row inIndexSet:(NSIndexSet *)indexSet
{
    unsigned currentIndex = [indexSet firstIndex];
    int i = 0;
    while (currentIndex != NSNotFound)
    {
		if (currentIndex < row) { i++; }
		currentIndex = [indexSet indexGreaterThanIndex:currentIndex];
    }
    return i;
}

@end
