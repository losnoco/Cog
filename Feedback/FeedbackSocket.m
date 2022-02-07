//
//  FeedbackSocket.m
//  Cog
//
//  Created by Vincent Spader on 3/27/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "FeedbackSocket.h"

#import "Logging.h"

@implementation FeedbackSocket

NSString *encodeForURL(NSString *s) {
	return (NSString *)CFBridgingRelease(CFURLCreateStringByAddingPercentEscapes(NULL, (CFStringRef)s, NULL, NULL, kCFStringEncodingUTF8));
}

- (void)sendFeedbackThread:(id)sender {
	@autoreleasepool {
		NSString *f = encodeForURL(from);
		NSString *s = encodeForURL(subject);
		NSString *m = encodeForURL(message);
		NSString *v = encodeForURL(version);

		NSString *postString = [NSString stringWithFormat:@"from=%@&subject=%@&message=%@&version=%@", f, s, m, v];

		NSData *postData = [postString dataUsingEncoding:NSASCIIStringEncoding];

		NSURL *url = [NSURL URLWithString:@"https://kode54.net/cog/feedback.php"];
		NSMutableURLRequest *post = [NSMutableURLRequest requestWithURL:url];

		[post addValue:@"application/x-www-form-urlencoded" forHTTPHeaderField:@"Content-Type"];
		[post setHTTPMethod:@"POST"];
		[post setHTTPBody:postData];

		NSError *error;
		NSURLResponse *response;
		NSData *resultData = [NSURLConnection sendSynchronousRequest:post returningResponse:&response error:&error];
		NSString *resultString = [[NSString alloc] initWithData:resultData encoding:NSASCIIStringEncoding];
		// DLog(@"RESULT: %@", resultString);
		if([resultString caseInsensitiveCompare:@"SUCCESS"] == NSOrderedSame) {
			[self performSelectorOnMainThread:@selector(returnSuccess:) withObject:nil waitUntilDone:NO];
		} else {
			[self performSelectorOnMainThread:@selector(returnFailure:) withObject:nil waitUntilDone:NO];
		}
	}
}

- (void)sendFeedback:(NSString *)f subject:(NSString *)s message:(NSString *)m version:(NSString *)v {
	if([f isEqualToString:@""]) {
		f = @"Anonymous";
	}
	[self setFrom:f];
	[self setSubject:s];
	[self setMessage:m];
	[self setVersion:v];

	[NSThread detachNewThreadSelector:@selector(sendFeedbackThread:) toTarget:self withObject:nil];
}

- (void)returnSuccess:(id)userInfo {
	if([delegate respondsToSelector:@selector(feedbackDidSend:)]) {
		[delegate feedbackDidSend:self];
	}
}

- (void)returnFailure:(id)userInfo {
	if([delegate respondsToSelector:@selector(feedbackDidNotSend:)]) {
		[delegate feedbackDidNotSend:self];
	}
}

- (void)setDelegate:(id<FeedbackSocketDelegate>)d {
	delegate = d;
}

- (void)setFrom:(NSString *)f {
	from = f;
}

- (void)setSubject:(NSString *)s {
	subject = s;
}

- (void)setMessage:(NSString *)m {
	message = m;
}

- (void)setVersion:(NSString *)v {
	version = v;
}

@end
