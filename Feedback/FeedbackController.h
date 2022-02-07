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
	IBOutlet NSTextField* fromView;
	IBOutlet NSTextField* subjectView;
	IBOutlet NSTextView* messageView;
	IBOutlet NSProgressIndicator* sendingIndicator;

	FeedbackSocket* feedbackSocket;
}

- (IBAction)sendFeedback:(id)sender;
- (IBAction)cancel:(id)sender;

@end
