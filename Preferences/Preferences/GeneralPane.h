//
//  GeneralPane.h
//  Preferences
//
//  Created by Christopher Snowhill on 6/20/22.
//

#import "GeneralPreferencePane.h"

#import "PathSuggester.h"
#import "SandboxPathBehaviorController.h"

NS_ASSUME_NONNULL_BEGIN

@interface GeneralPane : GeneralPreferencePane <NSTableViewDelegate> {
	IBOutlet SandboxPathBehaviorController *sandboxPathBehaviorController;
	IBOutlet PathSuggester *pathSuggester;
}

- (IBAction)addPath:(id)sender;
- (IBAction)deleteSelectedPaths:(id)sender;
- (IBAction)removeStaleEntries:(id)sender;
- (IBAction)showPathSuggester:(id)sender;
- (IBAction)refreshPathList:(id)sender;

@end

NS_ASSUME_NONNULL_END
