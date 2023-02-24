//
//  LyricsWindowController.h
//  Cog
//
//  Created by Christopher Snowhill on 2/23/23.
//

#import <Cocoa/Cocoa.h>

#import "AppController.h"

NS_ASSUME_NONNULL_BEGIN

@interface LyricsWindowController : NSWindowController {
	IBOutlet id playlistSelectionController;
	IBOutlet id currentEntryController;
	IBOutlet AppController *appController;

	id __unsafe_unretained valueToDisplay;
}

@property(assign) id valueToDisplay;

- (IBAction)toggleWindow:(id)sender;

@end

NS_ASSUME_NONNULL_END
