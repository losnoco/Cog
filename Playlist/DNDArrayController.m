
#import "DNDArrayController.h"

#import "Logging.h"

NSString *CogDNDIndexType = @"org.cogx.cog.dnd-index";
NSString *CogUrlsPboardType = @"org.cogx.cog.url";
NSString *iTunesDropType = @"com.apple.tv.metadata";

@implementation DNDArrayController

- (void)awakeFromNib {
	[super awakeFromNib];
	// register for drag and drop
	[self.tableView registerForDraggedTypes:@[CogDNDIndexType,
		                                      CogUrlsPboardType,
		                                      NSPasteboardTypeFileURL,
		                                      iTunesDropType]];
}

- (id<NSPasteboardWriting>)tableView:(NSTableView *)tableView
              pasteboardWriterForRow:(NSInteger)row {
	NSPasteboardItem *item = [NSPasteboardItem new];
	[item setString:[@(row) stringValue] forType:CogDNDIndexType];

	return item;
}

- (void)tableView:(NSTableView *)tableView
  draggingSession:(NSDraggingSession *)session
 willBeginAtPoint:(NSPoint)screenPoint
    forRowIndexes:(NSIndexSet *)rowIndexes {
	DLog(@"Drag session started with indexes: %@", rowIndexes);
}

- (NSDragOperation)tableView:(NSTableView *)tableView
                validateDrop:(id<NSDraggingInfo>)info
                 proposedRow:(NSInteger)row
       proposedDropOperation:(NSTableViewDropOperation)dropOperation {
	NSDragOperation dragOp = NSDragOperationCopy;

	if([info draggingSource] == tableView)
		dragOp = NSDragOperationMove;

	DLog(@"VALIDATING DROP!");
	// we want to put the object at, not over,
	// the current row (contrast NSTableViewDropOn)
	[tableView setDropRow:row dropOperation:NSTableViewDropAbove];

	return dragOp;
}

- (BOOL)tableView:(NSTableView *)tableView
       acceptDrop:(id<NSDraggingInfo>)info
              row:(NSInteger)row
    dropOperation:(NSTableViewDropOperation)dropOperation {
	if(row < 0) {
		row = 0;
	}

	NSArray<NSPasteboardItem *> *items = info.draggingPasteboard.pasteboardItems;
	// if drag source is self, it's a move
	if([info draggingSource] == tableView || items == nil) {
		NSMutableIndexSet *indexSet = [NSMutableIndexSet indexSet];
		for(NSPasteboardItem *item in items) {
			[indexSet addIndex:(NSUInteger)[[item stringForType:CogDNDIndexType] intValue]];
		}
		if([indexSet count] > 0) {
			DLog(@"INDEX SET ON DROP: %@", indexSet);
			NSArray *selected = [[self arrangedObjects] objectsAtIndexes:indexSet];
			[self moveObjectsInArrangedObjectsFromIndexes:indexSet toIndex:(unsigned int)row];

			[self setSelectedObjects:selected];

			DLog(@"ACCEPTING DROP!");
			return YES;
		}
	}
	DLog(@"REJECTING DROP!");
	return NO;
}

- (void)moveObjectsInArrangedObjectsFromIndexes:(NSIndexSet *)indexSet
                                        toIndex:(NSUInteger)insertIndex {
	__block NSUInteger rangeCount = 0;
	__block NSUInteger firstIndex = 0;
	[indexSet enumerateRangesUsingBlock:^(NSRange range, BOOL *_Nonnull stop) {
		if(++rangeCount == 1)
			firstIndex = range.location;
	}];

	if(rangeCount == 1 &&
	   (insertIndex >= firstIndex &&
	    insertIndex < firstIndex + [indexSet count])) // Null operation
		return;

	NSArray *objects = [self arrangedObjects];
	NSUInteger index = [indexSet lastIndex];

	NSUInteger aboveInsertIndexCount = 0;
	id object;
	NSUInteger removeIndex;

	while(NSNotFound != index) {
		if(index >= insertIndex) {
			removeIndex = index + aboveInsertIndexCount;
			aboveInsertIndexCount += 1;
		} else {
			removeIndex = index;
			insertIndex -= 1;
		}

		object = objects[removeIndex];

		[self removeObjectAtArrangedObjectIndex:removeIndex];
		[self insertObject:object atArrangedObjectIndex:insertIndex];

		index = [indexSet indexLessThanIndex:index];
	}
}

- (void)moveObjectsFromIndex:(NSUInteger)fromIndex
     toArrangedObjectIndexes:(NSIndexSet *)indexSet {
	__block NSUInteger rangeCount = 0;
	__block NSUInteger firstIndex = 0;
	__block NSUInteger _fromIndex = fromIndex;
	[indexSet enumerateRangesUsingBlock:^(NSRange range, BOOL *_Nonnull stop) {
		if(++rangeCount == 1)
			firstIndex = range.location;
		if(_fromIndex >= range.location) {
			if(_fromIndex < range.location + range.length)
				_fromIndex = range.location;
			else
				_fromIndex -= range.length;
		}
	}];

	if(rangeCount == 1 &&
	   (fromIndex >= firstIndex &&
	    fromIndex < firstIndex + [indexSet count])) // Null operation
		return;

	fromIndex = _fromIndex;

	NSArray *objects = [[self arrangedObjects] subarrayWithRange:NSMakeRange(fromIndex, [indexSet count])];
	NSUInteger index = [indexSet firstIndex];

	NSUInteger itemIndex = 0;
	id object;

	fromIndex += [objects count];
	for(NSUInteger i = 0; i < [objects count]; i++) {
		[self removeObjectAtArrangedObjectIndex:--fromIndex];
	}

	while(NSNotFound != index) {
		object = objects[itemIndex++];

		[self insertObject:object atArrangedObjectIndex:index];

		index = [indexSet indexGreaterThanIndex:index];
	}
}

@end
