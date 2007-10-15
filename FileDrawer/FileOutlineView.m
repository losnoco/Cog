//
//  FileOutlineView.m
//  BindTest
//
//  Created by Vincent Spader on 8/20/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "FileOutlineView.h"
#import "FileIconCell.h"
#import "FileTreeDataSource.h"


@interface FileOutlineView (KFTypeSelectTableViewSupport)
- (void)findPrevious:(id)sender;
- (void)findNext:(id)sender;
- (void)kfResetSearch;
@end

@implementation FileOutlineView

- (void) awakeFromNib
{
	NSEnumerator *e = [[self tableColumns] objectEnumerator];
	id c;
	while ((c = [e nextObject]))
	{
		id dataCell = [[FileIconCell alloc] init];
		
		[dataCell setLineBreakMode:NSLineBreakByTruncatingTail];
		[c setDataCell: dataCell];
		NSLog(@"Setting data cell!");
	}
}


- (BOOL)acceptsFirstResponder
{
	return YES;
}

- (BOOL)resignFirstResponder
{
	return YES;
}


//Navigate outline view with the keyboard, send select actions to delegate
- (void)keyDown:(NSEvent *)theEvent
{
	if (!([theEvent modifierFlags] & NSCommandKeyMask)) {
		
		NSString	*charString = [theEvent charactersIgnoringModifiers];
		unichar		pressedChar = 0;
		
		//Get the pressed character
		if ([charString length] == 1) pressedChar = [charString characterAtIndex:0];
		
    	if (pressedChar == NSDeleteFunctionKey || pressedChar == NSBackspaceCharacter || pressedChar == NSDeleteCharacter) { //Delete
			//As Weird-al said....EAT IT JUST EAT IT!!!
			[self kfResetSearch];
		} else if (pressedChar == NSCarriageReturnCharacter || pressedChar == NSEnterCharacter) { //Enter or return
			//Add songs to list
			//[[self delegate] addSelectedToPlaylist];
			
			[fileDrawer close];
		} else if (pressedChar == 0x1b) {//Escape
			[fileDrawer close];
		} else if (pressedChar == NSLeftArrowFunctionKey ||  pressedChar == NSRightArrowFunctionKey) { //left or right
			[super keyDown:theEvent];
	
			[self kfResetSearch];
		} else if ((pressedChar == '\031') && // backtab
			([self respondsToSelector:@selector(findPrevious:)])) {
			/* KFTypeSelectTableView supports findPrevious; backtab is added to AIOutlineView as a find previous action
			* if KFTypeSelectTableView is being used via posing */
			[self findPrevious:self];
			
		} else if ((pressedChar == '\t') &&
				   ([self respondsToSelector:@selector(findNext:)])) {
			/* KFTypeSelectTableView supports findNext; tab is added to AIOutlineView as a find next action
			* if KFTypeSelectTableView is being used via posing */
			[self findNext:self];
			
		} else {
			[super keyDown:theEvent];
		}
	} else {
		[super keyDown:theEvent];
	}
}

@end
