//
//  GeneralPane.h
//  Preferences
//
//  Created by Christopher Snowhill on 6/20/22.
//

#import "GeneralPreferencePane.h"

#import "SandboxPathBehaviorController.h"

NS_ASSUME_NONNULL_BEGIN

@interface GeneralPane : GeneralPreferencePane {
	IBOutlet SandboxPathBehaviorController *sandboxBehaviorController;
}

- (IBAction)addPath:(id)sender;
- (IBAction)deleteSelectedPaths:(id)sender;

@end

NS_ASSUME_NONNULL_END
