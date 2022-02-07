//
//  ToolTipTextField.m
//  Cog
//
//  Created by Vincent Spader on 3/9/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "ToolTipTextField.h"

@implementation ToolTipTextField

- (void)setObjectValue:(id)obj {
	NSString *string = [obj description];

	NSFormatter *formatter = [[self cell] formatter];
	if(nil != formatter) {
		string = [formatter stringForObjectValue:obj];
	}

	[self setToolTip:string];

	[super setObjectValue:obj];
}

@end
