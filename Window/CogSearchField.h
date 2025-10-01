//
//  CogSearchField.h
//  Cog
//
//  Created by Christopher Snowhill on 9/30/25.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@interface CogSearchField : NSSearchField<NSSearchFieldDelegate> {
	IBOutlet NSWindow *mainWindow;
}

@end

NS_ASSUME_NONNULL_END
