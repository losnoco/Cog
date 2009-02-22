//
//  VolumeSlider.m
//  Cog
//
//  Created by Vincent Spader on 2/8/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "VolumeSlider.h"
#import "PlaybackController.h"
#import "CogAudio/Helper.h"

@implementation VolumeSlider

- (id)initWithFrame:(NSRect)frame
{
	self = [super initWithFrame:frame];
	if (self)
	{
		toolTip = [[ToolTipWindow alloc] init];
	}
	
	return self;
}

- (id)initWithCoder:(NSCoder *)coder
{
	self = [super initWithCoder:coder];
	if (self)
	{
		toolTip = [[ToolTipWindow alloc] init];
	}
	
	return self;
}

- (void)updateToolTip
{
	double value = [self doubleValue];
	double volume = linearToLogarithmic(value);
	
	NSString *text = [[NSString alloc] initWithFormat:@"%0.lf", volume];
	
	NSSize size = [toolTip suggestedSizeForTooltip:text];
	NSPoint mouseLocation = [NSEvent mouseLocation];
	
	[toolTip setToolTip:text];
	[toolTip setFrame:NSMakeRect(mouseLocation.x, mouseLocation.y, size.width, size.height) display:YES];
	
	[text release];
}

- (void)showToolTip
{
	[self updateToolTip];
	
	[toolTip orderFront];
}

- (void)showToolTipForDuration:(NSTimeInterval)duration
{
	[self updateToolTip];
	[toolTip orderFrontForDuration:duration];
}


- (void)hideToolTip
{
	[toolTip close];
}


- (BOOL)sendAction:(SEL)theAction to:(id)theTarget
{
	double oneLog = logarithmicToLinear(100.0);
	double distance = [self frame].size.height*([self doubleValue] - oneLog)/100.0;
	if (fabs(distance) < 2.0)
	{
		[self setDoubleValue:oneLog];
	}

	[self showToolTip];
	
	return [super sendAction:theAction to:theTarget];
}

- (void)scrollWheel:(NSEvent *)theEvent
{
	double change = [theEvent deltaY];
	
	[self setDoubleValue:[self doubleValue] + change];
	
	[[self target] changeVolume:self];
	
	[self showToolTipForDuration:1.0];
}

@end
