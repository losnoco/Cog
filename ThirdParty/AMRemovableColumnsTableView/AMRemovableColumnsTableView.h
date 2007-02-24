//
//  AMRemovableColumnsTableView.h
//  HebX
//
//  Created by Andreas on 26.08.05.
//  Copyright 2005 Andreas Mayer. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface AMRemovableColumnsTableView : NSTableView {
	IBOutlet id obligatoryColumnIdentifiers; // NSArray
	NSSet *allTableColumns;
	NSSet *obligatoryTableColumns;
	BOOL am_respondsToControlDidBecomeFirstResponder;
}

- (NSSet *)allTableColumns;

- (NSSet *)visibleTableColumns;

- (NSSet *)hiddenTableColumns;

// obligatory columns are automatically retrieved from obligatoryColumnIdentifiers if not nil;
// use setter otherwise 
- (NSSet *)obligatoryTableColumns;
- (void)setObligatoryTableColumns:(NSSet *)newObligatoryTableColumns;

- (BOOL)isObligatoryColumn:(NSTableColumn *)column;

// use these to show and hide columns:

- (BOOL)hideTableColumn:(NSTableColumn *)column;

- (BOOL)showTableColumn:(NSTableColumn *)column;


@end
