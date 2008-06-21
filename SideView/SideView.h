//
//  HelperController.h
//  Cog
//
//  Created by Vincent Spader on 6/20/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class SideViewController;

@interface SideView : NSObject {
	IBOutlet NSView *view;
	IBOutlet NSResponder *firstResponder;
	
	SideViewController *controller;
}

- (id)initWithNibNamed:(NSString *)nibName controller:(SideViewController *)c;
- (NSView *)view;
- (NSResponder *)firstResponder;

- (void) addToPlaylist:(NSArray *)urls;

@end
