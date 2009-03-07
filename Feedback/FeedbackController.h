//
//  FeedbackController.h
//  Cog
//
//  Created by Vincent Spader on 3/26/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "FeedbackSocket.h"

@interface FeedbackController : NSWindowController<FeedbackSocketDelegate> {
	IBOutlet NSTextField* fromView;
	IBOutlet NSTextField* subjectView;
	IBOutlet NSTextView* messageView;
	IBOutlet NSProgressIndicator *sendingIndicator;
	
	FeedbackSocket *feedbackSocket;
}

- (IBAction)sendFeedback:(id)sender;
- (IBAction)cancel:(id)sender;

@end
