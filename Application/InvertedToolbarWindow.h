//
//  InvertedToolbarWindow.h
//  Cog
//
//  Created by Vincent Spader on 10/31/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface InvertedToolbarWindow : NSWindow {
	BOOL contentHidden;
	NSSize contentSize;
}

- (void)hideContent;
- (void)hideContentwithAnimation:(BOOL)animate;

- (void)showContent;


@end
