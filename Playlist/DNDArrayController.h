
#import <Cocoa/Cocoa.h>

extern NSString *CogDNDIndexType;
extern NSString *CogUrlsPboardType;
extern NSString *iTunesDropType;

@interface DNDArrayController : NSArrayController <NSTableViewDataSource>

@property IBOutlet NSTableView *tableView;

// table view drag and drop support
- (id <NSPasteboardWriting>)tableView:(NSTableView *)tableView
               pasteboardWriterForRow:(NSInteger)row;
- (void)tableView:(NSTableView *)tableView
  draggingSession:(NSDraggingSession *)session
 willBeginAtPoint:(NSPoint)screenPoint
    forRowIndexes:(NSIndexSet *)rowIndexes;
- (NSDragOperation)tableView:(NSTableView *)tableView
                validateDrop:(id <NSDraggingInfo>)info
                 proposedRow:(NSInteger)row
       proposedDropOperation:(NSTableViewDropOperation)dropOperation;
- (BOOL)tableView:(NSTableView *)tableView
       acceptDrop:(id <NSDraggingInfo>)info
              row:(NSInteger)row
    dropOperation:(NSTableViewDropOperation)dropOperation;

// utility methods
-(void)moveObjectsInArrangedObjectsFromIndexes:(NSIndexSet *)indexSet
                                       toIndex:(unsigned int)insertIndex;

@end
