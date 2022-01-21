//
//  MainWindow.m
//  Cog
//
//  Created by Vincent Spader on 2/22/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "MainWindow.h"


@implementation MainWindow

- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSWindowStyleMask)windowStyle backing:(NSBackingStoreType)bufferingType defer:(BOOL)deferCreation
{
    self = [super initWithContentRect:contentRect styleMask:windowStyle backing:bufferingType defer:deferCreation];
    if (self)
    {
        [self setExcludedFromWindowsMenu:YES];
        [self setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
    }
    return self;
}

- (void)awakeFromNib
{
    [super awakeFromNib];

    [playlistView setNextResponder:self];
    
    hdcdLogo = [NSImage imageNamed:@"hdcdLogoTemplate"];
    
    [self showHDCDLogo:NO];
}

- (void)showHDCDLogo:(BOOL)show
{
    for (NSToolbarItem * toolbarItem in [mainToolbar items])
    {
        if ([[toolbarItem itemIdentifier] isEqualToString:@"hdcd"]) {
            if (show)
                [toolbarItem setImage:hdcdLogo];
            else
                [toolbarItem setImage:nil];
        }
    }
}

@end
