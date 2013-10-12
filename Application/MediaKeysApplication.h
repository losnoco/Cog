//
//  MediaKeysApplication.h
//  Cog
//
//  Created by Vincent Spader on 10/3/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <IOKit/hidsystem/ev_keymap.h>

@class SPMediaKeyTap;

@interface MediaKeysApplication : NSApplication
{
    SPMediaKeyTap *keyTap;
}

@end

