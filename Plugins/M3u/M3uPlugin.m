//
//  M3uPlugin.m
//  M3u
//
//  Created by Zaphod Beeblebrox on 10/8/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "M3uPlugin.h"

#import "M3uContainer.h"

@implementation M3uPlugin

+ (NSDictionary *)pluginInfo
{
	return [NSDictionary dictionaryWithObject:kCogContainer forKey:[M3uContainer className]];
}

@end
