//
//  PreferencePluginController.h
//  Cog
//
//  Created by Vincent Spader on 3/7/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface PreferencePluginController : NSObject {
	NSArray *_plugins;

	NSMutableArray *_preferencePanes;
}

- (id)initWithPlugins:(NSArray *)plugins;

- (NSArray *)preferencePanes;

@end
