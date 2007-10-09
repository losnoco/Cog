//
//  PlsPlugin.m
//  Pls
//
//  Created by Zaphod Beeblebrox on 10/8/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "PlsPlugin.h"

#import "PlsContainer.h"

@implementation PlsPlugin

+ (NSDictionary *)pluginInfo
{
	return [NSDictionary dictionaryWithObject:kCogContainer forKey:[PlsContainer className]];
}

@end
