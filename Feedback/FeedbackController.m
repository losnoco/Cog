//
//  FeedbackController.m
//  Cog
//
//  Created by Vincent Spader on 3/26/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import "FeedbackController.h"


@implementation FeedbackController

- (IBAction)openFeedbackWindow:(id)sender
{	
	[fromView setStringValue:@""];
	[subjectView setStringValue:@""];
	[messageView setString:@""];
	
	[feedbackWindow makeFirstResponder:fromView];
	[feedbackWindow makeKeyAndOrderFront: sender];
}

- (void)alertDidEnd:(NSAlert *)alert returnCode:(int)returnCode contextInfo:(void *)contextInfo
{
	NSLog(@"CONTEXT: %i", contextInfo);
	if (contextInfo == YES)
	{
		[feedbackWindow close];
	}
}

- (void)FeedbackErrorOccurred:(NSNotification *)aNotification
{
	DBLog(@"Error sending feedback");
	
	[sendingIndicator stopAnimation:self];

	NSAlert *alert = [[[NSAlert alloc] init] autorelease];
	[alert setMessageText:@"Failed"];
	[alert setInformativeText:@"Feedback failed to send."];
	
	[alert beginSheetModalForWindow:feedbackWindow modalDelegate:self didEndSelector:@selector(alertDidEnd:returnCode:contextInfo:) contextInfo:NO];
}

- (void)FeedbackSent:(NSNotification *)aNotification
{
//	DBLog(@"Feedback Sent");

	[sendingIndicator stopAnimation:self];

	NSAlert *alert = [[[NSAlert alloc] init] autorelease];
	[alert setMessageText:@"Success"];
	[alert setInformativeText:@"Feedback successfully sent!"];

	[alert beginSheetModalForWindow:feedbackWindow modalDelegate:self didEndSelector:@selector(alertDidEnd:returnCode:contextInfo:) contextInfo:YES];
}


- (IBAction)sendFeedback:(id)sender
{
//	DBLog(@"Sending feedback...");

	[sendingIndicator startAnimation:self];
	
	//Using this so that if its a bad connection, it doesnt sit there looking stupid..or should it
	feedbackSocket = [[FeedbackSocket alloc] init];
	[feedbackSocket setDelegate:self];
	
	[feedbackSocket sendFeedback:[fromView stringValue] subject:[subjectView stringValue] message:[messageView string]];
}

- (IBAction)cancel:(id)sender
{
	[feedbackWindow close];
}

@end
