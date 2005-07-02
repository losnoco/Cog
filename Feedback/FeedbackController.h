//
//  FeedbackController.h
//  Cog
//
//  Created by Vincent Spader on 3/26/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "FeedbackSocket.h"

@interface FeedbackController : NSObject {
	IBOutlet NSWindow* feedbackWindow;
	IBOutlet NSTextField* fromView;
	IBOutlet NSTextField* subjectView;
	IBOutlet NSTextView* messageView;
	IBOutlet NSProgressIndicator *sendingIndicator;
	
	FeedbackSocket *feedbackSocket;
}

- (IBAction)openFeedbackWindow:(id)sender;
- (IBAction)sendFeedback:(id)sender;
- (IBAction)cancel:(id)sender;

@end
