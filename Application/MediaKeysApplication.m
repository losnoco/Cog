//
//  MediaKeysApplication.m
//  Cog
//
//  Created by Vincent Spader on 10/3/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "MediaKeysApplication.h"
#import "AppController.h"

@implementation MediaKeysApplication

- (void)mediaKeyEvent: (int)key state: (BOOL)state repeat: (BOOL)repeat
{
	NSLog(@"MEDIA EVENT KEY!");
	switch( key )
	{
		case NX_KEYTYPE_PLAY:
			if( state == 0 )
				[[self delegate] clickPlay]; //Play pressed and released
		break;
		
		case NX_KEYTYPE_NEXT:
		case NX_KEYTYPE_FAST:
			if( state == 0 )
				[[self delegate] clickNext]; //Next pressed and released
		break;
		
		case NX_KEYTYPE_PREVIOUS:
		case NX_KEYTYPE_REWIND:
			if( state == 0 )
				[[self delegate] clickPrev]; //Previous pressed and released
		break;
	}
}

- (void)sendEvent: (NSEvent*)event
{
	if( [event type] == NSSystemDefined && [event subtype] == 8 )
	{
		int keyCode = (([event data1] & 0xFFFF0000) >> 16);
		int keyFlags = ([event data1] & 0x0000FFFF);
		int keyState = (((keyFlags & 0xFF00) >> 8)) ==0xA;
		int keyRepeat = (keyFlags & 0x1);
		
		[self mediaKeyEvent: keyCode state: keyState repeat: keyRepeat];
	}

	[super sendEvent: event];
}
@end
