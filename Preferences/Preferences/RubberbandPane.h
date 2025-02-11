//
//  Rubberband.h
//  Preferences
//
//  Created by Christopher Snowhill on 2/9/25.
//

#ifndef Rubberband_h
#define Rubberband_h

#import "GeneralPreferencePane.h"
#import <Cocoa/Cocoa.h>

@interface RubberbandWindowArrayController : NSArrayController

- (void)reinitWithEngine:(BOOL)engineR3;

@end

@interface RubberbandPane : GeneralPreferencePane {
	IBOutlet RubberbandWindowArrayController *windowBehavior;
}

- (IBAction)changeState:(id)sender;

@end

@interface RubberbandEngineArrayController : NSArrayController

@end

@interface RubberbandTransientsArrayController : NSArrayController

@end

@interface RubberbandDetectorArrayController : NSArrayController

@end

@interface RubberbandPhaseArrayController : NSArrayController

@end

@interface RubberbandSmoothingArrayController : NSArrayController

@end

@interface RubberbandFormantArrayController : NSArrayController

@end

@interface RubberbandPitchArrayController : NSArrayController

@end

@interface RubberbandChannelsArrayController : NSArrayController

@end

#endif /* Rubberband_h */
