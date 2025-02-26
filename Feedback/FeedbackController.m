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
	BOOL showing;
	BOOL sent;
	
	NSString *name;
	NSString *email;
	NSString *comments;
}

- (id)init {
	self = [super initWithWindowNibName:@"Feedback"];
	if(self) {
		showing = NO;
		sent = NO;
	}
	return self;
}

- (IBAction)showWindow:(id)sender {
	[nameView setStringValue:@""];
	[emailView setStringValue:@""];
	[messageView setString:@""];

	[super showWindow:sender];
	
	showing = YES;
}

- (IBAction)sendFeedback:(id)sender {
	name = [nameView stringValue];
	email = [emailView stringValue];
	comments = [messageView string];

	[[self window] close];
	sent = YES;
	showing = NO;
}

- (IBAction)cancel:(id)sender {
	[[self window] close];
	sent = NO;
	showing = NO;
}

- (BOOL)waitForCompletion {
	while(showing) {
		usleep(2000);
	}
	return sent;
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
