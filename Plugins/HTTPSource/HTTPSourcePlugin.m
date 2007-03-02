//
//  HTTPSourcePlugin.m
//  FileSource
//
//  Created by Vincent Spader on 3/1/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "HTTPSourcePlugin.h"
#import "HTTPSource.h"

@implementation HTTPSourcePlugin 

+ (NSDictionary *)pluginInfo
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		kCogSource, [HTTPSource className],
		nil
	];
}


@end
