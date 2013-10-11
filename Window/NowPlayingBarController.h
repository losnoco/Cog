//
//  NowPlayingBarController.h
//  Cog
//
//  Created by Dmitry Promsky on 2/25/12.
//  Copyright 2012 dmitry.promsky@gmail.com. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface NowPlayingBarController : NSViewController
{
    IBOutlet NSTextField *text;
}

- (NSTextField*)text;

@end

