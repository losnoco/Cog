
#import <Cocoa/Cocoa.h>

extern NSString *MovedRowsType;
extern NSString *CogUrlsPbboardType;
extern NSString *iTunesDropType;

@interface DNDArrayController : NSArrayController
{
    IBOutlet NSTableView *tableView;
}

// table view drag and drop support
- (BOOL)tableView:(NSTableView *)tv writeRows:(NSArray*)rows toPasteboard:(NSPasteboard*)pboard;
- (NSDragOperation)tableView:(NSTableView*)tv validateDrop:(id <NSDraggingInfo>)info proposedRow:(int)row proposedDropOperation:(NSTableViewDropOperation)op;
- (BOOL)tableView:(NSTableView*)tv acceptDrop:(id <NSDraggingInfo>)info row:(int)row dropOperation:(NSTableViewDropOperation)op;
    

// utility methods
-(void)moveObjectsFromArrangedObjectIndexes:(NSArray *) sources toIndexes:(NSArray *)destinations;

-(void)moveObjectsInArrangedObjectsFromIndexes:(NSIndexSet*)indexSet toIndex:(unsigned int)insertIndex;

- (NSIndexSet *)indexSetFromRows:(NSArray *)rows;
- (int)rowsAboveRow:(int)row inIndexSet:(NSIndexSet *)indexSet;

@end
