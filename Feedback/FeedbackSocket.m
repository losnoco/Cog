//
//  FeedbackSocket.m
//  Cog
//
//  Created by Vincent Spader on 3/27/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "FeedbackSocket.h"

// NSNotifications
NSString *FeedbackErrorOccurredNotification = @"FeedbackErrorOccurredNotification";
NSString *FeedbackSentNotification = @"FeedbackSentNotification";

@implementation FeedbackSocket

NSString *encodeForURL(NSString *s)
{
	return [(NSString*) CFURLCreateStringByAddingPercentEscapes(NULL, (CFStringRef)s, NULL, NULL, kCFStringEncodingUTF8) autorelease];
}

- (void)sendFeedbackThread:(id)sender
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	NSString *f = encodeForURL(from);
	NSString *s = encodeForURL(subject);
	NSString *m = encodeForURL(message);
	
	NSString *postString = [NSString stringWithFormat:@"from=%@&subject=%@&message=%@", f, s, m];
	
	NSData *postData = [postString dataUsingEncoding:NSASCIIStringEncoding];
	
	NSURL *url = [NSURL URLWithString:@"http://cogx.org/feedback.php"];
	NSMutableURLRequest *post = [NSMutableURLRequest requestWithURL:url];
	
	[post addValue:@"application/x-www-form-urlencoded" forHTTPHeaderField:@"Content-Type"];
	[post setHTTPMethod:@"POST"];
	[post setHTTPBody:postData];
	
	NSError* error;
	NSURLResponse* response;
	NSData* resultData = [NSURLConnection sendSynchronousRequest:post returningResponse:&response error:&error];
	NSString *resultString = [[[NSString alloc] initWithData:resultData encoding:NSASCIIStringEncoding] autorelease];
	DBLog(@"RESULT: %@", resultString);
	if ([resultString caseInsensitiveCompare:@"SUCCESS"] == NSOrderedSame)
	{
		[self performSelectorOnMainThread:@selector(returnSuccess:) withObject:nil waitUntilDone:NO];
	}
	else
	{
		[self performSelectorOnMainThread:@selector(returnFailure:) withObject:nil waitUntilDone:NO];
	}
	
	[pool release];
}

- (void)sendFeedback: (NSString *)f subject:(NSString *)s message:(NSString *)m
{
//	DBLog(@"Detaching thread for feedback");
	if ([f isEqualToString:@""])
	{
		f = @"Anonymous";
	}
	[self setFrom:f];
	[self setSubject:s];
	[self setMessage:m];
	
    [NSThread detachNewThreadSelector:@selector(sendFeedbackThread:) toTarget:self withObject:nil];
}

- (void)returnSuccess:(id)userInfo
{
    [[NSNotificationCenter defaultCenter] postNotificationName:FeedbackSentNotification object:self];
}

- (void)returnFailure:(id)userInfo
{
    [[NSNotificationCenter defaultCenter] postNotificationName:FeedbackErrorOccurredNotification object:self];
}

-(void)setDelegate:(id)d
{
	NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
	
    if (delegate != nil) {
        // Unregister with the notification center
        [nc removeObserver:delegate name:FeedbackErrorOccurredNotification object:self];
        [nc removeObserver:delegate name:FeedbackSentNotification object:self];
        [delegate autorelease];
    }
    delegate = [d retain];
    
	// Register the new FeedbackNotification methods for the delegate
    // Only register if the delegate implements it, though
    if ([delegate respondsToSelector:@selector(FeedbackErrorOccurred:)])
	{
        [nc addObserver:delegate selector:@selector(FeedbackErrorOccurred:) name:FeedbackErrorOccurredNotification object:self];
    }
    if ([delegate respondsToSelector:@selector(FeedbackSent:)])
	{
        [nc addObserver:delegate selector:@selector(FeedbackSent:) name:FeedbackSentNotification object:self];
    }
}


- (void)setFrom:(NSString *)f
{
	[f retain];
	[from release];
	
	from = f;
}

- (void)setSubject:(NSString *)s
{
	[s retain];
	[subject release];
	
	subject = s;
}

- (void)setMessage:(NSString *)m
{
	[m retain];
	[message release];
	
	message = m;
}

@end
