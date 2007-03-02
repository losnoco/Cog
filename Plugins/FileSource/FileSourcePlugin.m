//
//  FileSourcePlugin.m
//  FileSource
//
//  Created by Vincent Spader on 3/1/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "FileSourcePlugin.h"
#import "FileSource.h"

@implementation FileSourcePlugin 

+ (NSDictionary *)pluginInfo
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		kCogSource, [FileSource className],
		nil
	];
}


@end
