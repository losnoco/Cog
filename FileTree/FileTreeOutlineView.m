//
//  FileTreeOutlineView.m
//  Cog
//
//  Created by Vincent Spader on 6/21/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "FileTreeOutlineView.h"
#import "FileTreeViewController.h"

@implementation FileTreeOutlineView

- (void)awakeFromNib
{
	[self setDoubleAction:@selector(addToPlaylist:)];
	[self setTarget:[self delegate]];
}

- (void)keyDown:(NSEvent *)e
{
    unsigned int   modifiers = [e modifierFlags] & (NSCommandKeyMask | NSShiftKeyMask | NSControlKeyMask | NSAlternateKeyMask);
    NSString       *characters = [e characters];
	unichar        c;
	
	if ([characters length] == 1) 
	{
		c = [characters characterAtIndex:0];
		
		if (modifiers == 0 && (c == NSEnterCharacter || c == NSCarriageReturnCharacter))
		{
			[[self delegate] addToPlaylist:self];

			return;
		}
	}
	
	[super keyDown:e];

	return;
}

@end
