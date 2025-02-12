//
//  SilenceDecoder.h
//  Cog
//
//  Created by Christopher Snowhill on 2/8/15.
//  Copyright 2015 __NoWork, LLC__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "Plugin.h"

@interface SilenceDecoder : NSObject <CogDecoder> {
	id<CogSource> source;

	long length;
	long remain;

	double seconds;

	float *buffer;
}

- (void)setSource:(id<CogSource>)s;
- (id<CogSource>)source;
@end
