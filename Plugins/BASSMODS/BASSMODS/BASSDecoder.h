//
//  BASSDecoder.h
//  Cog
//
//  Created by Christopher Snowhill on 7/03/14.
//  Copyright 2014 __NoWork, Inc__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "bass.h"

#import "Plugin.h"

@interface BASSDecoder : NSObject <CogDecoder> {
	HMUSIC music;
    
	id<CogSource> source;
	long length;
    
    long loops;
    long fadeTotal;
    long fadeRemain;
}

- (void)setSource:(id<CogSource>)s;
- (id<CogSource>)source;
- (void)cleanUp;
- (void)sync;
@end
