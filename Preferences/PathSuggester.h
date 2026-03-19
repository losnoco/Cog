//
//  PathSuggester.h
//  Preferences
//
//  Created by Christopher Snowhill on 6/21/22.
//

#import <Cocoa/Cocoa.h>

#import "SandboxPathBehaviorController.h"

NS_ASSUME_NONNULL_BEGIN

@interface PathSuggesterView : NSArrayController<NSTableViewDelegate, NSTableViewDataSource> {
    IBOutlet SandboxPathBehaviorController *sandboxPathBehaviorController;
    IBOutlet NSWindow *window;
}
@end

@interface PathSuggester : NSWindowController {
	IBOutlet SandboxPathBehaviorController *sandboxPathBehaviorController;
    IBOutlet PathSuggesterView *viewController;
}

- (IBAction)beginSuggestion:(id)sender;

@end

NS_ASSUME_NONNULL_END
