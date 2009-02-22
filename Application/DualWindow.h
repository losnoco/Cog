//
//  InvertedToolbarWindow.h
//  Cog
//
//  Created by Vincent Spader on 10/31/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface DualWindow : NSWindow {
	IBOutlet DualWindow *otherWindow;
}

- (void)show;

- (NSString *)hiddenDefaultsKey;
- (BOOL)isHidden;
- (void)setHidden:(BOOL)h;

@end
