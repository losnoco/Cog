//
//  SpectrumItem.m
//  Cog
//
//  Created by Christopher Snowhill on 2/13/22.
//

#import "SpectrumItem.h"

#import "SpectrumView.h"
#import "SpectrumViewLegacy.h"

@implementation SpectrumItem

- (void)awakeFromNib {
	NSRect frame = NSMakeRect(0, 0, 64, 26);
	NSView *view = [[SpectrumView alloc] initWithFrame:frame];
	if(!view) view = [[SpectrumViewLegacy alloc] initWithFrame:frame];
	[self setView:view];
}

@end
