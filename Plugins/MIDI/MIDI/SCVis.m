//
//  SCVis.m
//  MIDI
//
//  Created by Christopher Snowhill on 9/25/25.
//

#import <Foundation/Foundation.h>

#import "SCVis.h"

static NSString *CogSCVisUpdateNotification = @"CogSCVisUpdateNotification";

@implementation SCVisUpdate

- (id)initWithFile:(NSURL *)file whichScreen:(uint32_t)which state:(const void *)state stateSize:(size_t)size timestamp:(uint64_t)timestamp {
	self = [super init];
	if(self) {
		self.file = file;
		self.timestamp = timestamp;
		self.which = which;
		self.state = [NSData dataWithBytes:state length:size];
	}
	return self;
}

+ (void)post:(SCVisUpdate *)update {
	[[NSNotificationCenter defaultCenter] postNotificationName:CogSCVisUpdateNotification object:update];
}

@end
