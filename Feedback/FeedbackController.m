//
//  FeedbackController.m
//  Cog
//
//  Created by Vincent Spader on 3/26/05.
//  Copyright 2005 Vincent Spader All rights reserved.
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
	if ([(NSNumber *)contextInfo boolValue]== YES)
	{
		[feedbackWindow close];
	}
}

- (void)FeedbackErrorOccurred:(NSNotification *)aNotification
{
	DBLog(@"Error sending feedback");
	
	[sendingIndicator stopAnimation:self];

	NSAlert *alert = [[[NSAlert alloc] init] autorelease];
	[alert setMessageText:NSLocalizedString(@"FeedbackFailedMessageText", @"")];
	[alert setInformativeText:NSLocalizedString(@"FeedbackFailedInformativeText", @"")];
	
	[alert beginSheetModalForWindow:feedbackWindow modalDelegate:self didEndSelector:@selector(alertDidEnd:returnCode:contextInfo:) contextInfo:[NSNumber numberWithBool:NO]];
}

- (void)FeedbackSent:(NSNotification *)aNotification
{
//	DBLog(@"Feedback Sent");

	[sendingIndicator stopAnimation:self];

	NSAlert *alert = [[[NSAlert alloc] init] autorelease];
	[alert setMessageText:NSLocalizedString(@"FeedbackSuccessMessageText", @"")];
	[alert setInformativeText:NSLocalizedString(@"FeedbackSuccessInformativeText", @"")];

	[alert beginSheetModalForWindow:feedbackWindow modalDelegate:self didEndSelector:@selector(alertDidEnd:returnCode:contextInfo:) contextInfo:[NSNumber numberWithBool:YES]];
}


- (IBAction)sendFeedback:(id)sender
{
//	DBLog(@"Sending feedback...");

	[sendingIndicator startAnimation:self];
	
	//Using this so that if its a bad connection, it doesnt sit there looking stupid..or should it
	feedbackSocket = [[FeedbackSocket alloc] init];
	[feedbackSocket setDelegate:self];

	NSString *version = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleVersion"];
	
	[feedbackSocket sendFeedback:[fromView stringValue] subject:[subjectView stringValue] message:[messageView string] version:version];
}

- (IBAction)cancel:(id)sender
{
	[feedbackWindow close];
}

@end
