//
//  DumbFile.h
//  Cog
//
//  Created by Vincent Spader on 5/29/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <Dumb/dumb.h>

#define __FRAMEWORK__
#import <Dumb/it.h>
#undef __FRAMEWORK__

#import "Plugin.h"

@interface DumbDecoder : NSObject <CogDecoder> {
	DUMBFILE *df;
	DUMBFILE_SYSTEM	dfs;
	DUH *duh;
	DUH_SIGRENDERER *dsr;
	
	id<CogSource> source;
	long length;
}

- (void)setSource:(id<CogSource>)s;
- (id<CogSource>)source;
- (void)cleanUp;
@end
