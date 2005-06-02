//
//  PlaylistView.m
//  Cog
//
//  Created by Vincent Spader on 3/20/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import "PlaylistView.h"
#import "SoundController.h"
#import "PlaylistController.h"

@implementation PlaylistView

- (BOOL)acceptsFirstResponder
{
	return YES;
}
- (BOOL)resignFirstResponder
{
	return YES;
}

- (void)mouseDown:(NSEvent *)e
{
//	DBLog(@"MOUSE DOWN");
	if ([e type] == NSLeftMouseDown && [e clickCount] == 2)
	{
		[soundController playEntryAtIndex:[self selectedRow]];
	}
	else
	{
//		DBLog(@"Super");
		[super mouseDown:e];
	}
}


- (void)keyDown:(NSEvent *)e
{
	NSString *s;
	unichar c;
	
	s = [e charactersIgnoringModifiers];
	if ([s length] != 1)
		return;

	c = [s characterAtIndex:0];
	if (c == NSDeleteCharacter)
	{
		[playlistController remove:self];
	}
	else if (c == ' ')
	{
		[soundController pauseResume:self];
	}
	else if (c == NSEnterCharacter || c == NSCarriageReturnCharacter)
	{
		[soundController playEntryAtIndex:[self selectedRow]];
	}
	else
	{
		[super keyDown:e];
	}
}

@end
