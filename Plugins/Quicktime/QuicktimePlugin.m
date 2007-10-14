//
//  QuicktimePlugin.m
//  Quicktime
//
//  Created by Vincent Spader on 6/10/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "QuicktimePlugin.h"
#import "QuicktimeDecoder.h"

@implementation QuicktimePlugin

+ (NSDictionary *)pluginInfo
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		kCogDecoder, 			[QuicktimeDecoder className],
		nil
	];
}

@end
