//
//  DumbFile.h
//  Cog
//
//  Created by Vincent Spader on 5/29/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <Dumb/dumb.h>
#import <Dumb/it.h>

#import "Plugin.h"

extern DUMBFILE *dumbfile_open_memory_and_free(char *data, long size);

@interface DumbDecoder : NSObject <CogDecoder> {
	DUH *duh;
	DUH_SIGRENDERER *dsr;
	
	id<CogSource> source;
	long length;
    
    long loops;
    long fadeTotal;
    long fadeRemain;
}

- (void)setSource:(id<CogSource>)s;
- (id<CogSource>)source;
- (void)cleanUp;
@end
