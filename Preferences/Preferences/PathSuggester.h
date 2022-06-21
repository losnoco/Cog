//
//  PathSuggester.h
//  Preferences
//
//  Created by Christopher Snowhill on 6/21/22.
//

#import <Cocoa/Cocoa.h>

#import "SandboxPathBehaviorController.h"

NS_ASSUME_NONNULL_BEGIN

@interface PathSuggester : NSWindowController <NSTableViewDelegate> {
	IBOutlet SandboxPathBehaviorController *sandboxPathBehaviorController;
	IBOutlet NSArrayController *pathsList;
}

- (IBAction)beginSuggestion:(id)sender;

@end

NS_ASSUME_NONNULL_END
