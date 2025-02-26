//
//  FeedbackController.h
//  Cog
//
//  Created by Vincent Spader on 3/26/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "FeedbackSocket.h"
#import <Cocoa/Cocoa.h>

@interface FeedbackController : NSWindowController <FeedbackSocketDelegate> {
	IBOutlet NSTextField* nameView;
	IBOutlet NSTextField* emailView;
	IBOutlet NSTextView* messageView;
}

- (IBAction)sendFeedback:(id)sender;
- (IBAction)cancel:(id)sender;

- (BOOL)waitForCompletion;

- (NSString *)name;
- (NSString *)email;
- (NSString *)comments;

@end
