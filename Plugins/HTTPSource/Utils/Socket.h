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
	int _fd;
}

+ (id)socketWithHost:(NSString *)host port:(unsigned int) port;
- (id)initWithHost:(NSString *)host port:(unsigned int)port;

- (int)send:(const void *)data amount:(unsigned int)amount;
- (int)receive:(void *)data amount:(unsigned int)amount;
- (void)close;

@end
