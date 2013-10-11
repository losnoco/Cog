//
//  NowPlayingBarView.m
//  Cog
//
//  Created by Dmitry Promsky on 2/24/12.
//  Copyright 2012 dmitry.promsky@gmail.com. All rights reserved.
//

#import "NowPlayingBarView.h"

@implementation NowPlayingBarView

- (id)initWithFrame:(NSRect)rect
{
    if ((self = [super initWithFrame: rect]))
    {
        NSColor *lightColor = [NSColor colorWithCalibratedRed: 160.0/255.0 green: 160.0/255.0 blue: 160.0/255.0 alpha: 1.0];
        NSColor *darkColor = [NSColor colorWithCalibratedRed: 155.0/255.0 green: 155.0/255.0 blue: 155.0/255.0 alpha: 1.0];
        gradient = [[NSGradient alloc] initWithStartingColor: lightColor endingColor: darkColor];
    }
    return self;
}

- (void)drawRect:(NSRect)rect
{
    const BOOL active = [[self window] isMainWindow];
    
    NSInteger count = 0;
    NSRect gridRects[2];
    NSColor * colorRects[2];
    
    NSRect lineBorderRect = NSMakeRect(NSMinX(rect), NSHeight([self bounds]) - 1.0, NSWidth(rect), 1.0);
    
    if (NSIntersectsRect(lineBorderRect, rect))
    {
        gridRects[count] = lineBorderRect;
        colorRects[count] = [NSColor colorWithCalibratedWhite: 0.75 alpha: 1.0];
        ++count;
        
        rect.size.height -= 1.0;
    }
    
    lineBorderRect.origin.y = 0.0;
    if (NSIntersectsRect(lineBorderRect, rect))
    {
        gridRects[count] = lineBorderRect;
        colorRects[count] = active ? [NSColor colorWithCalibratedWhite: 0.25 alpha: 1.0]
        : [NSColor colorWithCalibratedWhite: 0.5 alpha: 1.0];
        ++count;
        
        rect.origin.y += 1.0;
        rect.size.height -= 1.0;
    }
    
    if (!NSIsEmptyRect(rect))
    {
        const NSRect gradientRect = NSMakeRect(NSMinX(rect), 1.0, NSWidth(rect), NSHeight([self bounds]) - 1.0 - 1.0); //proper gradient requires the full height of the bar
        [gradient drawInRect: gradientRect angle: 270.0];
    }
    
    NSRectFillListWithColors(gridRects, colorRects, count);
}

- (void)dealloc
{
    [gradient release];
    [super dealloc];
}

@end

