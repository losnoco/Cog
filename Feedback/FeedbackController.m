//
//  FeedbackController.m
//  Cog
//
//  Created by Vincent Spader on 3/26/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "FeedbackController.h"

#import "Logging.h"

@implementation FeedbackController

- (id)init
{
	return [super initWithWindowNibName:@"Feedback"];
}

- (IBAction)showWindow:(id)sender
{	
	[fromView setStringValue:@""];
	[subjectView setStringValue:@""];
	[messageView setString:@""];
	
	[super showWindow:sender];
}

- (void)alertDidEnd:(NSAlert *)alert returnCode:(int)returnCode contextInfo:(void *)contextInfo
{
	if ([(NSNumber *)CFBridgingRelease(contextInfo) boolValue]== YES)
	{
		[[self window] close];
	}
}

- (void)feedbackDidNotSend:(FeedbackSocket *)feedback
{
	ALog(@"Error sending feedback");
	
	[sendingIndicator stopAnimation:self];

	NSAlert *alert = [[NSAlert alloc] init];
	[alert setMessageText:NSLocalizedString(@"FeedbackFailedMessageText", @"")];
	[alert setInformativeText:NSLocalizedString(@"FeedbackFailedInformativeText", @"")];
	
	[alert beginSheetModalForWindow:[self window] modalDelegate:self didEndSelector:@selector(alertDidEnd:returnCode:contextInfo:) contextInfo:(void *)CFBridgingRetain([NSNumber numberWithBool:NO])];
}

- (void)feedbackDidSend:(FeedbackSocket *)feedback
{
	[sendingIndicator stopAnimation:self];

	NSAlert *alert = [[NSAlert alloc] init];
	[alert setMessageText:NSLocalizedString(@"FeedbackSuccessMessageText", @"")];
	[alert setInformativeText:NSLocalizedString(@"FeedbackSuccessInformativeText", @"")];

	[alert beginSheetModalForWindow:[self window] modalDelegate:self didEndSelector:@selector(alertDidEnd:returnCode:contextInfo:) contextInfo:(void *)CFBridgingRetain([NSNumber numberWithBool:YES])];
}


- (IBAction)sendFeedback:(id)sender
{
	[sendingIndicator startAnimation:self];
	
	//Using this so that if its a bad connection, it doesnt sit there looking stupid..or should it
	feedbackSocket = [[FeedbackSocket alloc] init];
	[feedbackSocket setDelegate:self];

	NSString *version = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleVersion"];
	
	[feedbackSocket sendFeedback:[fromView stringValue] subject:[subjectView stringValue] message:[messageView string] version:version];
}

- (IBAction)cancel:(id)sender
{
	[[self window] close];
}

@end
