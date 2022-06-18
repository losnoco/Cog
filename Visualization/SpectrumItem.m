//
//  SpectrumItem.m
//  Cog
//
//  Created by Christopher Snowhill on 2/13/22.
//

#import "SpectrumItem.h"

#import "SpectrumViewSK.h"
#import "SpectrumViewCG.h"

#import "Logging.h"

@implementation SpectrumItem

- (void)awakeFromNib {
	NSRect frame = NSMakeRect(0, 0, 64, 26);
	NSView *view = nil;
	if(self.toolbar) view = [SpectrumViewSK createGuardWithFrame:frame];
	if(!view) view = [[SpectrumViewCG alloc] initWithFrame:frame];
	[self setView:view];
}

@end
