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
        gradient_light = [[NSGradient alloc] initWithStartingColor: lightColor endingColor: darkColor];
        
        lightColor = [NSColor colorWithCalibratedRed: 56.0/255.0 green: 56.0/255.0 blue: 56.0/255.0 alpha: 1.0];
        darkColor = [NSColor colorWithCalibratedRed: 64.0/255.0 green: 64.0/255.0 blue: 64.0/255.0 alpha: 1.0];
        gradient_dark = [[NSGradient alloc] initWithStartingColor: lightColor endingColor: darkColor];
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
    
    Boolean isDarkMode = NO;
    if (@available(macOS 10.14, *)) {
        NSAppearance * appearance = [[NSApplication sharedApplication] effectiveAppearance];
        NSAppearanceName basicAppearance = [appearance bestMatchFromAppearancesWithNames:@[NSAppearanceNameAqua, NSAppearanceNameDarkAqua]];
        if ([basicAppearance isEqualToString:NSAppearanceNameDarkAqua]) {
            isDarkMode = YES;
        }
    }
    
    if (NSIntersectsRect(lineBorderRect, rect))
    {
        gridRects[count] = lineBorderRect;
        if (isDarkMode)
            colorRects[count] = [NSColor colorWithCalibratedWhite: 0.40 alpha: 1.0];
        else
            colorRects[count] = [NSColor colorWithCalibratedWhite: 0.75 alpha: 1.0];
        ++count;
        
        rect.size.height -= 1.0;
    }
    
    lineBorderRect.origin.y = 0.0;
    if (NSIntersectsRect(lineBorderRect, rect))
    {
        gridRects[count] = lineBorderRect;
        if (isDarkMode)
            colorRects[count] = active ? [NSColor colorWithCalibratedWhite: 0.60 alpha: 1.0] : [NSColor colorWithCalibratedWhite: 0.4 alpha: 1.0];
        else
            colorRects[count] = active ? [NSColor colorWithCalibratedWhite: 0.25 alpha: 1.0]
            : [NSColor colorWithCalibratedWhite: 0.5 alpha: 1.0];
        ++count;
        
        rect.origin.y += 1.0;
        rect.size.height -= 1.0;
    }
    
    if (!NSIsEmptyRect(rect))
    {
        const NSRect gradientRect = NSMakeRect(NSMinX(rect), 1.0, NSWidth(rect), NSHeight([self bounds]) - 1.0 - 1.0); //proper gradient requires the full height of the bar
        if (isDarkMode)
            [gradient_dark drawInRect: gradientRect angle: 270.0];
        else
            [gradient_light drawInRect: gradientRect angle: 270.0];
    }
    
    NSRectFillListWithColors(gridRects, colorRects, count);
}

@end

