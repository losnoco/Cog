//
//  FeedbackController.m
//  Cog
//
//  Created by Vincent Spader on 3/26/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "FeedbackController.h"

#import "Logging.h"

@implementation FeedbackController {
	BOOL finished;
	
	NSString *name;
	NSString *email;
	NSString *comments;
}

- (id)init {
	self = [super initWithWindowNibName:@"Feedback"];
	if(self) {
		finished = NO;
	}
	return self;
}

- (IBAction)showWindow:(id)sender {
	[nameView setStringValue:@""];
	[emailView setStringValue:@""];
	[messageView setString:@""];

	[[self window] setDelegate:self];
	[super showWindow:sender];
}

/// Reports how the window was done with, once.
- (void)finish:(BOOL)sent {
	if(finished) return;
	finished = YES;
	void (^handler)(BOOL) = self.completionHandler;
	self.completionHandler = nil;
	if(handler) handler(sent);
}

// Closed with its close button: as cancelled.
- (void)windowWillClose:(NSNotification *)notification {
	[self finish:NO];
}

- (IBAction)sendFeedback:(id)sender {
	name = [nameView stringValue];
	email = [emailView stringValue];
	comments = [messageView string];

	[self finish:YES];
	[[self window] close];
}

- (IBAction)cancel:(id)sender {
	[self finish:NO];
	[[self window] close];
}

- (NSString *)name {
	return name;
}

- (NSString *)email {
	return email;
}

- (NSString *)comments {
	return comments;
}

@end
