//
//  InputNode.h
//  Cog
//
//  Created by Vincent Spader on 8/2/05.
//  Copyright 2005 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "Node.h"
#import "Plugin.h"

@interface SourceNode : Node <CogSource>
{
	id<CogSource> source;
	
	long byteCount;
}

- (id)initWithSource:(id<CogSource>)source;

- (void)process;


//Same interface as the CogSource protocol
- (BOOL)open:(NSURL *)url;

- (int)read:(void *)buf amount:(int)amount;

- (BOOL)seekable;
- (BOOL)seek:(long)offset whence:(int)whence;

- (long)tell;

- (void)close;

- (NSDictionary *) properties;
//End of protocol interface
@end
