//
//  SpotlightPlaylistView.m
//  Cog
//
//  Created by Matthew Grinshpun on 12/02/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "SpotlightPlaylistView.h"


@implementation SpotlightPlaylistView

- (void)awakeFromNib
{
    [super awakeFromNib];
    
    // We don't want the font to be bold
    NSFontManager *fontManager = [NSFontManager sharedFontManager];
    
    for(NSTableColumn *c in self.tableColumns)
    {
        [c.dataCell setFont:[fontManager convertFont:[c.dataCell font]
                                       toHaveTrait:NSUnboldFontMask]];
    }
}

@end
