//
//  MiniPlayerPlusWindowController.h
//  Cog
//

#import <Cocoa/Cocoa.h>

#import "AppController.h"

@interface MiniPlayerPlusWindowController : NSWindowController <NSWindowDelegate> {
	IBOutlet id playlistSelectionController;
	IBOutlet id currentEntryController;
	IBOutlet AppController *appController;

	id __unsafe_unretained valueToDisplay;
}

@property(nonatomic, assign) id valueToDisplay;

- (void)rebuildContent;
- (NSTextField *)labelFieldWithString:(NSString *)str font:(NSFont *)font width:(CGFloat)w atY:(CGFloat)y;
- (CGFloat)addRow:(NSString *)label value:(NSString *)value toView:(NSView *)parent atY:(CGFloat)y width:(CGFloat)width;

@end

@interface _FlippedView : NSView
@end
