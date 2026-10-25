//
//  FeedbackController.h
//  Cog
//
//  Created by Vincent Spader on 3/26/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "FeedbackSocket.h"
#import <Cocoa/Cocoa.h>

@interface FeedbackController : NSWindowController <NSWindowDelegate> {
	IBOutlet NSTextField* nameView;
	IBOutlet NSTextField* emailView;
	IBOutlet NSTextView* messageView;
}

- (IBAction)sendFeedback:(id)sender;
- (IBAction)cancel:(id)sender;

/// Called once, on the main thread, when the window is done with: YES if
/// the feedback was sent, NO if it was cancelled or closed.
@property(nonatomic, copy) void (^_Nullable completionHandler)(BOOL sent);

- (NSString *)name;
- (NSString *)email;
- (NSString *)comments;

@end
