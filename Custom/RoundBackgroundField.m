//
//  RoundBackgroundField.m
//  Cog
//
//  Created by Zaphod Beeblebrox on 4/28/06.
//  Copyright 2006 __MyCompanyName__. All rights reserved.
//

#import "RoundBackgroundField.h"
#import "RoundBackgroundCell.h"

@implementation RoundBackgroundField

- (id)initWithCoder:(NSCoder *)decoder
{
	self = [super initWithCoder:decoder];
	if (self)
	{
		id cell = [[RoundBackgroundCell alloc] init];
		id oldcell = [self cell];
		
		[cell setBackgroundColor: [self backgroundColor]];
		[cell setDrawsBackground: NO];
		
		[cell setScrollable:[oldcell isScrollable]];
		[cell setAlignment:[oldcell alignment]];
		[cell setLineBreakMode:[oldcell lineBreakMode]];
		
		[cell setAction: [oldcell action]];
		[cell setTarget: [oldcell target]];
		
		[cell setStringValue: [oldcell stringValue]];
		
		[self setCell: cell];
		[cell release];
	}
	
	return self;
}

@end
