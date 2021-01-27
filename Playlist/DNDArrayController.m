
#import "DNDArrayController.h"

#import "Logging.h"

@implementation DNDArrayController

NSString *CogPlaylistItemType = @"org.cogx.cog.playlist-item";
NSString *CogUrlsPboardType = @"COG_URLS_TYPE";

// @"CorePasteboardFlavorType 0x6974756E" is the "itun" type representing an iTunes plist
NSString *iTunesDropType = @"CorePasteboardFlavorType 0x6974756E";

- (void)awakeFromNib
{
    // register for drag and drop
    [self.tableView registerForDraggedTypes:@[CogPlaylistItemType, CogUrlsPboardType,
                                         NSFilenamesPboardType, iTunesDropType]];
}


- (id <NSPasteboardWriting>)tableView:(NSTableView *)tableView
               pasteboardWriterForRow:(NSInteger)row
{
    NSPasteboardItem *item = [[NSPasteboardItem alloc] init];
    [item setString:[@(row) stringValue] forType:CogPlaylistItemType];
    
    return item;
}

- (void)tableView:(NSTableView *)tableView
     draggingSession:(NSDraggingSession *)session
    willBeginAtPoint:(NSPoint)screenPoint
       forRowIndexes:(NSIndexSet *)rowIndexes
{
    DLog(@"Drag session started with indexes: %@", rowIndexes);
}


- (NSDragOperation)tableView:(NSTableView*)tableView
                validateDrop:(id <NSDraggingInfo>)info
                 proposedRow:(int)row
       proposedDropOperation:(NSTableViewDropOperation)dropOperation
{
    NSDragOperation dragOp = NSDragOperationCopy;

    if ([info draggingSource] == tableView)
        dragOp = NSDragOperationMove;

    DLog(@"VALIDATING DROP!");
    // we want to put the object at, not over,
    // the current row (contrast NSTableViewDropOn)
    [tableView setDropRow:row dropOperation:NSTableViewDropAbove];

    return dragOp;
}


- (BOOL)tableView:(NSTableView*)tableView
       acceptDrop:(id <NSDraggingInfo>)info
              row:(int)row
    dropOperation:(NSTableViewDropOperation)dropOperation
{
    if (row < 0) {
        row = 0;
    }
    
    NSArray<NSPasteboardItem *> *items = info.draggingPasteboard.pasteboardItems;
    // if drag source is self, it's a move
    if ([info draggingSource] == tableView || items == nil) {
        NSMutableIndexSet *indexSet = [NSMutableIndexSet indexSet];
        for (NSPasteboardItem *item in items) {
            [indexSet addIndex:[[item stringForType:CogPlaylistItemType] intValue]];
        }
        if ([indexSet count] > 0) {
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


-(void) moveObjectsInArrangedObjectsFromIndexes:(NSIndexSet *)indexSet
                                        toIndex:(unsigned int)insertIndex
{
    NSArray     *objects = [self arrangedObjects];
    NSUInteger  index = [indexSet lastIndex];

    int         aboveInsertIndexCount = 0;
    id          object;
    int         removeIndex;

    while (NSNotFound != index) {
        if (index >= insertIndex) {
            removeIndex = (int)(index + aboveInsertIndexCount);
            aboveInsertIndexCount += 1;
        } else {
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
