//
//  SpectrumItem.m
//  Cog
//
//  Created by Christopher Snowhill on 2/13/22.
//

#import "SpectrumItem.h"

@implementation SpectrumItem

- (void)awakeFromNib {
	SpectrumView *view = [[SpectrumView alloc] initWithFrame:NSMakeRect(0, 0, 64, 26)];
	[self setView:view];
}

@end
