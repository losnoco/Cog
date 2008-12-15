//
//  TagEditorController.m
//  Cog
//
//  Created by Safari on 08/11/17.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "TagEditorController.h"
#import "PlaylistController.h"


@implementation TagEditorController

- (id)init
{
    self = [super init];
	
    if( self )
    {
        // initialize the class
    }
	
    return self;
}

- (void)showInfoForURL:(NSURL*)url
{
	[tageditorWindow showWindow:self];
}

- (IBAction)cancel:(id)sender
{
	[tageditorWindow close];
}

@end
