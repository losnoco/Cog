//
//  SilenceSource.h
//  SilenceSource
//
//  Created by Christopher Snowhill on 2/8/15.
//  Copyright 2015 __NoWork, LLC__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "Plugin.h"

@interface SilenceSource : NSObject <CogSource>
{
	NSURL *_url;
}

@end
