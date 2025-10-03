//
//  InfoWindowController.m
//  Cog
//
//  Created by Vincent Spader on 3/7/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "InfoWindowController.h"
#import "AppController.h"
#import "Logging.h"
#import "MissingAlbumArtTransformer.h"
#import "PlaylistEntry.h"

@implementation InfoWindowController

static void *kInfoWindowControllerContext = &kInfoWindowControllerContext;

@synthesize valueToDisplay;

+ (void)initialize {
	NSValueTransformer *missingAlbumArtTransformer = [MissingAlbumArtTransformer new];
	[NSValueTransformer setValueTransformer:missingAlbumArtTransformer
	                                forName:@"MissingAlbumArtTransformer"];
}

- (id)init {
	return [super initWithWindowNibName:@"InfoInspector"];
}

- (void)awakeFromNib {
	[playlistSelectionController addObserver:self forKeyPath:@"selection" options:NSKeyValueObservingOptionNew context:kInfoWindowControllerContext];
	[currentEntryController addObserver:self forKeyPath:@"content" options:NSKeyValueObservingOptionNew context:kInfoWindowControllerContext];
	[appController addObserver:self forKeyPath:@"miniMode" options:NSKeyValueObservingOptionNew context:kInfoWindowControllerContext];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
	if(context == kInfoWindowControllerContext) {
		// Avoid "selection" because it creates a proxy that's hard to reason with when we don't need to write.
		PlaylistEntry *currentSelection = [[playlistSelectionController selectedObjects] firstObject];
		if(currentSelection != NULL) {
			[self setValueToDisplay:currentSelection];
		} else {
			[self setValueToDisplay:[currentEntryController content]];
		}
	} else {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
	}
}

- (IBAction)toggleWindow:(id)sender {
	if([[self window] isVisible])
		[[self window] orderOut:self];
	else {
		if([NSApp mainWindow]) {
			NSRect rect = [[NSApp mainWindow] frame];
			// Align Info Inspector HUD Panel to the right of Main Window.
			NSPoint point = NSMakePoint(NSMaxX(rect), NSMaxY(rect));
			[[self window] setFrameTopLeftPoint:point];
		}
		[self showWindow:self];
	}
}

@end
