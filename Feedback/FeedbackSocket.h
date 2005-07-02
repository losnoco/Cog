//
//  FeedbackSocket.h
//  Cog
//
//  Created by Vincent Spader on 3/27/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface FeedbackSocket : NSObject {
	NSString *from;
	NSString *subject;
	NSString *message;
	
	id delegate;
}

- (void)setDelegate:(id)d;
- (void)sendFeedback: (NSString *)f subject:(NSString *)s message:(NSString *)m;

- (void)setFrom:(NSString *)f;
- (void)setSubject:(NSString *)s;
- (void)setMessage:(NSString *)m;

@end
