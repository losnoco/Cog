//
//  InfoWindowController.m
//  Cog
//
//  Created by Vincent Spader on 3/7/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "InfoWindowController.h"
#import "MissingAlbumArtTransformer.h"
#import "Logging.h"
#import "AppController.h"

@implementation InfoWindowController

@synthesize valueToDisplay;

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
		
- (void)awakeFromNib
{
    [playlistSelectionController addObserver:self forKeyPath:@"selection" options:NSKeyValueObservingOptionNew context:nil];
    [currentEntryController addObserver:self forKeyPath:@"content" options:NSKeyValueObservingOptionNew context:nil];
    [appController addObserver:self forKeyPath:@"miniMode" options:NSKeyValueObservingOptionNew context:nil];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    BOOL miniMode = [appController miniMode];
    BOOL currentEntryChanged = (object == currentEntryController && [keyPath isEqualTo:@"content"]);
    BOOL selectionChanged = (object == playlistSelectionController && [keyPath isEqualTo:@"selection"]);
    BOOL miniModeSwitched = (object == appController && [keyPath isEqual:@"miniMode"]);

    if (miniMode && (currentEntryChanged || miniModeSwitched))
    {
        [self setValueToDisplay:[currentEntryController content]];
    }
    else if (!miniMode && (selectionChanged || miniModeSwitched))
    {
        [self setValueToDisplay:[playlistSelectionController selection]];
    }
}

- (IBAction)toggleWindow:(id)sender
{
	if ([[self window] isVisible])
		[[self window] orderOut:self];
	else
		[self showWindow:self];
}

@end
