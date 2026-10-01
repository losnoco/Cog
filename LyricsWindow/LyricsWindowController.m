//
//  LyricsWindowController.m
//  Cog
//
//  Created by Christopher Snowhill on 2/23/23.
//

#import "LyricsWindowController.h"

#import "AppController.h"
#import "PlaylistEntry.h"

#import "Cog-Swift.h"

@interface LyricsWindowController () {
	// What LRCLIB said about the displayed entry, and the question it was
	// asked, so a change of tags asks again.
	NSString *lrclibText;
	NSString *lrclibQuestion;
}

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
	[[NSUserDefaults standardUserDefaults] addObserver:self forKeyPath:@"enableLrclib" options:NSKeyValueObservingOptionNew context:kLyricsWindowControllerContext];
}

+ (NSSet *)keyPathsForValuesAffectingLyricsText {
	return [NSSet setWithObjects:@"valueToDisplay", @"valueToDisplay.unsyncedlyrics", @"valueToDisplay.rawTitle", @"valueToDisplay.artist", @"valueToDisplay.album", @"valueToDisplay.length", nil];
}

// The entry's own lyrics win; otherwise whatever LRCLIB has for it, asked
// only while the window is showing.
- (NSString *)lyricsText {
	PlaylistEntry *entry = valueToDisplay;
	NSString *own = [entry unsyncedlyrics];
	if([own length] || !entry) {
		return own;
	}

	NSString *question = [NSString stringWithFormat:@"%@\n%@\n%@\n%@", [entry rawTitle], [entry artist], [entry album], [entry length]];
	if(![question isEqualToString:lrclibQuestion] && [[self window] isVisible]) {
		// Not from inside the getter: the answer may arrive synchronously
		// and change this very property.
		dispatch_async(dispatch_get_main_queue(), ^{
			[self askLrclibFor:entry question:question];
		});
	}
	return lrclibText;
}

- (void)askLrclibFor:(PlaylistEntry *)entry question:(NSString *)question {
	if(entry != valueToDisplay || [question isEqualToString:lrclibQuestion]) {
		return;
	}
	lrclibQuestion = question;

	__weak LyricsWindowController *weakSelf = self;
	NSString *text = [[CogLyricsLookup shared] displayTextWithTitle:[entry rawTitle]
	                                                         artist:[entry artist]
	                                                          album:[entry album]
	                                                       duration:[[entry length] doubleValue]
	                                                     completion:^(NSString *answer) {
		LyricsWindowController *strongSelf = weakSelf;
		if(strongSelf && [question isEqualToString:strongSelf->lrclibQuestion]) {
			[strongSelf setLrclibText:answer];
		}
	}];
	[self setLrclibText:text];
}

- (void)setLrclibText:(NSString *)text {
	[self willChangeValueForKey:@"lyricsText"];
	lrclibText = text;
	[self didChangeValueForKey:@"lyricsText"];
}

- (void)forgetLrclib {
	lrclibQuestion = nil;
	[self setLrclibText:nil];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
	if(context == kLyricsWindowControllerContext) {
		if([keyPath isEqualToString:@"enableLrclib"]) {
			[self forgetLrclib];
			return;
		}
		// Avoid "selection" because it creates a proxy that's hard to reason with when we don't need to write.
		PlaylistEntry *currentSelection = [[playlistSelectionController selectedObjects] firstObject];
		id entry = currentSelection != NULL ? currentSelection : [currentEntryController content];
		if(entry != valueToDisplay) {
			lrclibQuestion = nil;
			lrclibText = nil;
			[self setValueToDisplay:entry];
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
		// Lookups wait for the window to be visible.
		[self forgetLrclib];
	}
}



@end
