
#import <Cocoa/Cocoa.h>

extern NSString *CogPlaylistItemType;
extern NSString *CogUrlsPboardType;
extern NSString *iTunesDropType;

@interface DNDArrayController : NSArrayController <NSTableViewDataSource>

@property IBOutlet NSTableView *tableView;

// table view drag and drop support
- (id <NSPasteboardWriting>)tableView:(NSTableView *)tableView
               pasteboardWriterForRow:(NSInteger)row;
- (NSDragOperation)tableView:(NSTableView *)tableView
                validateDrop:(id <NSDraggingInfo>)info
                 proposedRow:(int)row
       proposedDropOperation:(NSTableViewDropOperation)dropOperation;
- (BOOL)tableView:(NSTableView *)tableView
       acceptDrop:(id <NSDraggingInfo>)info
              row:(int)row
    dropOperation:(NSTableViewDropOperation)dropOperation;

// utility methods
-(void)moveObjectsInArrangedObjectsFromIndexes:(NSIndexSet *)indexSet
                                       toIndex:(unsigned int)insertIndex;

@end
