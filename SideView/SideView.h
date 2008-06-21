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
	SideViewController *controller;
}

- (id)initWithNibNamed:(NSString *)nibName controller:(SideViewController *)c;
- (NSView *)view;

- (void) addToPlaylist:(NSArray *)urls;

@end
