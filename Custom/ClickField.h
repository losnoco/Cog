/* ClickField */

#import <Cocoa/Cocoa.h>
#import "SoundController.h"

@interface ClickField : NSTextField
{
	IBOutlet SoundController *soundController;
}
@end
