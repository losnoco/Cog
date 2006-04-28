//
//  PlaylistView.m
//  Cog
//
//  Created by Vincent Spader on 3/20/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "PlaylistView.h"
#import "PlaybackController.h"
#import "PlaylistController.h"

@implementation PlaylistView

- (void)awakeFromNib
{
	id c;
	NSControlSize s = NSSmallControlSize;
	NSEnumerator *oe = [[self tableColumns] objectEnumerator];
	NSFont *f = [NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:s]];
	
	[self setRowHeight:[f defaultLineHeightForFont]];

	//Resize the fonts
	while (c = [oe nextObject])
	{
		[[c dataCell] setControlSize:s];
		[[c dataCell] setFont:f];
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

- (BOOL)acceptsFirstMouse:(NSEvent *)mouseDownEvent
{
	return NO;
}

- (void)mouseDown:(NSEvent *)e
{
//	DBLog(@"MOUSE DOWN");
	if ([e type] == NSLeftMouseDown && [e clickCount] == 2)
	{
		[playbackController play:self];
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
		[playbackController playPauseResume:self];
	}
	else if (c == NSEnterCharacter || c == NSCarriageReturnCharacter)
	{
		[playbackController play:self];
	}
	else
	{
		[super keyDown:e];
	}
}

@end
