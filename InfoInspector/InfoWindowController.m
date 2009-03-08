//
//  InfoWindowController.m
//  Cog
//
//  Created by Vincent Spader on 3/7/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "InfoWindowController.h"
#import "MissingAlbumArtTransformer.h"

@implementation InfoWindowController

@synthesize playlistSelectionController;

+ (void)initialize
{
	NSValueTransformer *missingAlbumArtTransformer = [[[MissingAlbumArtTransformer alloc] init] autorelease];
    [NSValueTransformer setValueTransformer:missingAlbumArtTransformer
                                    forName:@"MissingAlbumArtTransformer"];
}

- (id)init
{
	return [super initWithWindowNibName:@"InfoInspector"];
}
		
- (IBAction)toggleWindow:(id)sender
{
	if ([[self window] isVisible])
		[[self window] orderOut:self];
	else
		[self showWindow:self];
}

@end
