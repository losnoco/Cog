//
//  PreferencePluginController.m
//  Cog
//
//  Created by Vincent Spader on 3/7/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "PreferencePluginController.h"
#import "PreferencePanePlugin.h"

@implementation PreferencePluginController

- (id)initWithPlugins:(NSArray *)plugins {
	self = [super init];
	if(self) {
		_plugins = plugins;
		_preferencePanes = [NSMutableArray new];
	}

	return self;
}

- (void)_searchForPlugins {
	for(NSBundle *plugin in _plugins) {
		[plugin load];

		Class principalClass = [plugin principalClass];
		if([principalClass conformsToProtocol:@protocol(PreferencePanePlugin)]) {
			NSArray *panes = [principalClass preferencePanes];

			[_preferencePanes addObjectsFromArray:panes];
		}
	}
}

- (NSArray *)preferencePanes {
	[self _searchForPlugins];

	return _preferencePanes;
}

@end
