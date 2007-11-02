//
//  GCPopTestView.h
//  GCWindowMenuTest
//
//  Created by Graham on Tue Apr 24 2007. Modified from NSView to NSButton by Vincent Spader.
//  Copyright (c) 2007 __MyCompanyName__. All rights reserved.
//

#import <AppKit/AppKit.h>


@interface PopupButton : NSButton
{
	NSImage* _popButton;
	IBOutlet NSView *_popView;
	BOOL _hilited;
}

@end
