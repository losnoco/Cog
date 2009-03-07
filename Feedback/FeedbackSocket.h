//
//  FeedbackSocket.h
//  Cog
//
//  Created by Vincent Spader on 3/27/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>

@protocol FeedbackSocketDelegate;

@interface FeedbackSocket : NSObject {
	NSString *from;
	NSString *subject;
	NSString *message;
	NSString *version;
	
	id<FeedbackSocketDelegate> delegate;
}

- (void)setDelegate:(id<FeedbackSocketDelegate>)d;
- (void)sendFeedback: (NSString *)f subject:(NSString *)s message:(NSString *)m version:(NSString *)v;

- (void)setFrom:(NSString *)f;
- (void)setSubject:(NSString *)s;
- (void)setMessage:(NSString *)m;
- (void)setVersion:(NSString *)v;

@end

@protocol FeedbackSocketDelegate<NSObject>

- (void)feedbackDidSend:(FeedbackSocket *)feedback;
- (void)feedbackDidNotSend:(FeedbackSocket *)feedback;

@end
