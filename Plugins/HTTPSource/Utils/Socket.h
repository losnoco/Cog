//
//  Socket.h
//  Cog
//
//  Created by Vincent Spader on 2/28/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

//Rediculously simple socket wrapper
@interface Socket : NSObject {
	int _socket;
}

+ (id)socketWithHost:(NSString *)host port:(int) port;
- (id)initWithHost:(NSString *)host port:(int)port;

- (NSInteger)send:(const void *)data amount:(NSInteger)amount;
- (NSInteger)receive:(void *)data amount:(NSInteger)amount;
- (void)close;

@end
