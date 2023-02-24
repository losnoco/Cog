//
//  LyricsWindowController.m
//  Cog
//
//  Created by Christopher Snowhill on 2/23/23.
//

#import "LyricsWindowController.h"

#import "AppController.h"
#import "PlaylistEntry.h"

@interface LyricsWindowController ()

@end

@implementation LyricsWindowController

static void *kLyricsWindowControllerContext = &kLyricsWindowControllerContext;

@synthesize valueToDisplay;

- (id)init {
	return [super initWithWindowNibName:@"LyricsWindow"];
}

- (void)awakeFromNib {
	[playlistSelectionController addObserver:self forKeyPath:@"selection" options:NSKeyValueObservingOptionNew context:kLyricsWindowControllerContext];
	[currentEntryController addObserver:self forKeyPath:@"content" options:NSKeyValueObservingOptionNew context:kLyricsWindowControllerContext];
	[appController addObserver:self forKeyPath:@"miniMode" options:NSKeyValueObservingOptionNew context:kLyricsWindowControllerContext];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
	if(context == kLyricsWindowControllerContext) {
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
			// Align Lyrics Window to the right of Main Window.
			NSPoint point = NSMakePoint(NSMaxX(rect), NSMaxY(rect));
			[[self window] setFrameTopLeftPoint:point];
		}
		[self showWindow:self];
	}
}



@end
