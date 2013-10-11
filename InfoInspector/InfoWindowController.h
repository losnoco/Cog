//
//  InfoWindowController.h
//  Cog
//
//  Created by Vincent Spader on 3/7/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "AppController.h"


@interface InfoWindowController : NSWindowController
{
	IBOutlet id playlistSelectionController;
    IBOutlet id currentEntryController;
    IBOutlet AppController *appController;

    id valueToDisplay;
}

@property(assign) id valueToDisplay;

- (IBAction)toggleWindow:(id)sender;

@end
