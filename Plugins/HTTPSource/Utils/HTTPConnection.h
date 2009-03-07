//
//  HTTPConnection.h
//  HTTPSource
//
//  Created by Vincent Spader on 3/6/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "Socket.h"

@interface HTTPConnection : NSObject {
	Socket *_socket;
	NSURL *_URL;

	NSMutableDictionary *_requestHeaders;
	NSMutableDictionary *_responseHeaders;
	
	uint8_t *_buffer;
	NSInteger _bufferSize;
}

- (id)initWithURL:(NSURL *)url;

- (BOOL)connect;
- (void)close;

- (NSInteger)receiveData:(void *)bytes amount:(NSInteger)amount;

- (void)setValue:(NSString *)value forRequestHeader:(NSString *)header;
- (NSString *)valueForResponseHeader:(NSString *)header;

@property(copy) NSURL *URL;

@end
