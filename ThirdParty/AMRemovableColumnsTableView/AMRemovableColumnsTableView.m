//
//  AMRemovableColumnsTableView.m
//  HebX
//
//  Created by Andreas on 26.08.05.
//  Copyright 2005 Andreas Mayer. All rights reserved.
//

#import "AMRemovableColumnsTableView.h"
#import "AMRemovableTableColumn.h"


@interface NSTableView (ApplePrivate)
- (void)_readPersistentTableColumns;
- (void)_writePersistentTableColumns;
@end


@interface AMRemovableColumnsTableView (Private)
- (NSString *)columnVisibilitySaveName;
- (void)setAllTableColumns:(NSSet *)newAllTableColumns;
- (void)am_hideTableColumn:(NSTableColumn *)column;
- (void)am_showTableColumn:(NSTableColumn *)column;
- (void)readPersistentTableColumns;
- (void)writePersistentTableColumns;
- (void)am_readPersistentTableColumns;
- (void)am_writePersistentTableColumns;
@end


@implementation AMRemovableColumnsTableView

static BOOL AMRemovableColumnsTableView_readPersistentTableColumnsIsPublic = NO;

+ (void)initialize
{
	// should the framework support readPersistentTableColumns/writePersistentTableColumns, use the public methods
	AMRemovableColumnsTableView_readPersistentTableColumnsIsPublic = [NSTableColumn instancesRespondToSelector:@selector(readPersistentTableColumns)];
}

- (void)awakeFromNib
{
	am_respondsToControlDidBecomeFirstResponder = [[self delegate] respondsToSelector:@selector(controlDidBecomeFirstResponder:)];

	// set main table view for columns
	NSEnumerator *enumerator = [[self tableColumns] objectEnumerator];
	AMRemovableTableColumn *column;
	while (column = [enumerator nextObject]) {
		[column setMainTableView:self];
	}
	
	// if there's an array of the names of obligatory columns, update the obligatoryTableColumns set
	if (obligatoryColumnIdentifiers) {
		NSMutableSet *columns = (NSMutableSet *)[NSMutableSet setWithSet:[self allTableColumns]];
		NSEnumerator *enumerator = [columns objectEnumerator];
		NSTableColumn *column;
		while (column = [enumerator nextObject]) {
			if (![(NSArray *)obligatoryColumnIdentifiers containsObject:[column identifier]]) {
				[columns removeObject:column];
			}
		}
		[self setObligatoryTableColumns:columns];
	}
}

- (void)dealloc
{
	[obligatoryColumnIdentifiers release];
	[super dealloc];
}


- (NSSet *)allTableColumns
{
	return allTableColumns; 
}

- (void)setAllTableColumns:(NSSet *)newAllTableColumns
{
	if (allTableColumns != newAllTableColumns) {
		[newAllTableColumns retain];
		[allTableColumns release];
		allTableColumns = newAllTableColumns;
	}
}

- (NSSet *)visibleTableColumns
{
	NSMutableSet *result = [NSMutableSet set];
	[result addObjectsFromArray:[self tableColumns]];
	return result;
}

- (NSSet *)hiddenTableColumns
{
	NSMutableSet *result = [NSMutableSet setWithSet:[self allTableColumns]];
	[result minusSet:[self visibleTableColumns]];
	return result;
}

- (NSSet *)obligatoryTableColumns
{
	return obligatoryTableColumns; 
}

- (void)setObligatoryTableColumns:(NSSet *)newObligatoryTableColumns
{
	if (obligatoryTableColumns != newObligatoryTableColumns) {
		[newObligatoryTableColumns retain];
		[obligatoryTableColumns release];
		obligatoryTableColumns = newObligatoryTableColumns;
	}
}

- (BOOL)hideTableColumn:(NSTableColumn *)column
{
	BOOL result = NO;
	if (![(AMRemovableTableColumn *)column isHidden] && ![self isObligatoryColumn:column]) {
		[(AMRemovableTableColumn *)column setHidden:YES];
		result = YES;
	}
	return result;
}

- (BOOL)showTableColumn:(NSTableColumn *)column
{
	BOOL result = NO;
	if ([(AMRemovableTableColumn *)column isHidden]) {
		[(AMRemovableTableColumn *)column setHidden:NO];
		result = YES;
	}
	return result;
}

- (BOOL)isObligatoryColumn:(NSTableColumn *)column
{
	return [(NSArray *)obligatoryColumnIdentifiers containsObject:[column identifier]]	;
}


// ============================================================
#pragma mark -
#pragma mark ━ table view methods ━
// ============================================================

- (NSTableColumn *)tableColumnWithIdentifier:(id)identifier
{
	NSTableColumn *result = nil;
	NSEnumerator *enumerator = [[self allTableColumns] objectEnumerator];
	NSTableColumn *column;
	while (column = [enumerator nextObject]) {
		if ([[column identifier] isEqualToString:identifier]) {
			result = column;
			break;
		}
	}
	return result;
}

- (void)setDelegate:(id)anObject
{
	am_respondsToControlDidBecomeFirstResponder = [anObject respondsToSelector:@selector(controlDidBecomeFirstResponder:)];
	[super setDelegate:anObject];
}


// ============================================================
#pragma mark -
#pragma mark ━ view methods ━
// ============================================================

- (void)viewWillMoveToWindow:(NSWindow *)newWindow
{
	//NSLog(@"AMRemovableColumnsTableView - viewWillMoveToWindow:%@", newWindow);
	[self am_readPersistentTableColumns];
	[super viewWillMoveToWindow:newWindow];
}


// ============================================================
#pragma mark -
#pragma mark ━ responder methods ━
// ============================================================

- (BOOL)becomeFirstResponder
{
	BOOL result = [super becomeFirstResponder];
	if (result && am_respondsToControlDidBecomeFirstResponder) {
		[[self delegate] performSelector:@selector(controlDidBecomeFirstResponder:) withObject:self];
	}
	return result;
}


// ============================================================
#pragma mark -
#pragma mark ━ private table view methods ━
// ============================================================

- (void)_readPersistentTableColumns
{
	//NSLog(@"_readPersistentTableColumns");
	if (!AMRemovableColumnsTableView_readPersistentTableColumnsIsPublic) {
		[self setAllTableColumns:[NSSet setWithArray:[self tableColumns]]];
		// restore visible columns
		[self am_readPersistentTableColumns];
	} // else  readPersistentTableColumns will be used
	[super _readPersistentTableColumns];
}

- (void)_writePersistentTableColumns
{
	//NSLog(@"_writePersistentTableColumns");
	[super _writePersistentTableColumns];
	// save visible columns
	if (!AMRemovableColumnsTableView_readPersistentTableColumnsIsPublic) {
		[self am_writePersistentTableColumns];
	} // else  writePersistentTableColumns will be used
}

// just in case these methods should become public:
- (void)readPersistentTableColumns
{
	//NSLog(@"_readPersistentTableColumns");
	[self setAllTableColumns:[NSSet setWithArray:[self tableColumns]]];
	// restore visible columns
	[self am_readPersistentTableColumns];
	[(id)super readPersistentTableColumns]; // cast to avoid compiler warning
}

- (void)writePersistentTableColumns
{
	//NSLog(@"_writePersistentTableColumns");
	[(id)super writePersistentTableColumns]; // cast to avoid compiler warning
	// save visible columns
	[self am_writePersistentTableColumns];
}


// ============================================================
#pragma mark -
#pragma mark ━ private methods ━
// ============================================================

- (NSString *)columnVisibilitySaveName
{
	NSString *autosaveName = [self autosaveName];
	if (!autosaveName) {
		NSLog(@"AMRemovableColumnsTableView: autosave name missing for table view: %@", self);
		autosaveName = @"no-autosave-name-set";
	}
	return [@"AMRemovableColumnsTableView VisibleColumns " stringByAppendingString:autosaveName];
}

- (void)am_readPersistentTableColumns
{
	// restore visible columns
	NSArray *visibleColumnIdentifiers = [[NSUserDefaults standardUserDefaults] objectForKey:[self columnVisibilitySaveName]];
	if (visibleColumnIdentifiers) {
		NSSet *allColumns = [self allTableColumns];
		NSEnumerator *enumerator = [allColumns objectEnumerator];
		NSTableColumn *column;
		while (column = [enumerator nextObject]) {
			if (![visibleColumnIdentifiers containsObject:[column identifier]]) {
				[(AMRemovableTableColumn *)column setMainTableView:self];
				[self removeTableColumn:column];
			}
		}
	}
}

- (void)am_writePersistentTableColumns
{
	NSMutableArray *visibleColumnIdentifiers = [NSMutableArray array];
	NSEnumerator *enumerator = [[self visibleTableColumns] objectEnumerator];
	NSTableColumn *column;
	while (column = [enumerator nextObject]) {
		[visibleColumnIdentifiers addObject:[column identifier]];
	}
	[[NSUserDefaults standardUserDefaults] setObject:visibleColumnIdentifiers forKey:[self columnVisibilitySaveName]];
}

- (void)am_hideTableColumn:(NSTableColumn *)column
{
	[(AMRemovableTableColumn *)column setMainTableView:self];
	[self removeTableColumn:column];
	[NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(am_writePersistentTableColumns) object:nil];
	[self performSelector:@selector(am_writePersistentTableColumns) withObject:nil afterDelay:0.0];
}

- (void)am_showTableColumn:(NSTableColumn *)column
{
	[self addTableColumn:column];
	[NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(am_writePersistentTableColumns) object:nil];
	[self performSelector:@selector(am_writePersistentTableColumns) withObject:nil afterDelay:0.0];
}


@end
