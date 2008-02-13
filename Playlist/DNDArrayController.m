
#import "DNDArrayController.h"

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


- (BOOL)tableView:(NSTableView *)tv
		writeRows:(NSArray*)rows
	 toPasteboard:(NSPasteboard*)pboard
{
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

    // if drag source is self, it's a move
    if ([info draggingSource] == tableView)
	{
		NSArray *rows = [NSKeyedUnarchiver unarchiveObjectWithData:[[info draggingPasteboard] dataForType: MovedRowsType]];
	
		NSIndexSet  *indexSet = [self indexSetFromRows:rows];
	
		[self moveObjectsInArrangedObjectsFromIndexes:indexSet toIndex:row];
	
		// set selected rows to those that were just moved
		// Need to work out what moved where to determine proper selection...
		
		return YES;
	}
	
	return NO;
}


-(void)moveObjectsFromArrangedObjectIndexes:(NSArray *) sources toIndexes:(NSArray *)destinations;
{
	//We expect [sources count] == [destinations count].
	NSMutableArray *selectedObjects = [[NSMutableArray alloc] init];
	
	NSUInteger i = 0;
	for (i = 0; i < [sources count]; i++) {
		NSUInteger source = [[sources objectAtIndex:i] unsignedIntegerValue];
		NSUInteger dest = [[destinations objectAtIndex:i] unsignedIntegerValue];
		
		id object = [[self arrangedObjects] objectAtIndex:source];

		[object retain];

		[self removeObjectAtArrangedObjectIndex:source];
		[self insertObject:object atArrangedObjectIndex:dest];
		
		[selectedObjects addObject: object];

		[object release];
	}
	
	[self setSelectedObjects:selectedObjects];

	[selectedObjects release];
}

-(void)moveObjectsInArrangedObjectsFromIndexes:(NSIndexSet*)indexSet
										toIndex:(unsigned int)insertIndex
{
	int			index = [indexSet lastIndex];
    int			aboveInsertIndexCount = 0;
    int			removeIndex;

	NSMutableArray *sources = [NSMutableArray array];
	NSMutableArray *destinations = [NSMutableArray array];
	
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
		
		[sources addObject:[NSNumber numberWithUnsignedInteger:removeIndex]];
		[destinations addObject: [NSNumber numberWithUnsignedInteger:insertIndex]];
		
		index = [indexSet indexLessThanIndex:index];
    }
	
	[self moveObjectsFromArrangedObjectIndexes:sources toIndexes:destinations];
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

@end
