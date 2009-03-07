//
//  InfoWindowController.h
//  Cog
//
//  Created by Vincent Spader on 3/7/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface InfoWindowController : NSWindowController {
	IBOutlet id playlistSelectionController;
}

@property(readonly) id playlistSelectionController;

- (IBAction)toggleWindow:(id)sender;

@end
