//
//  MiniWindow.m
//  Cog
//
//  Created by Vincent Spader on 2/22/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "MiniWindow.h"

#import <Carbon/Carbon.h>

extern NSString *iTunesDropType;

extern void showSentryConsent(NSWindow *window);

@implementation MiniWindow

- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSWindowStyleMask)windowStyle backing:(NSBackingStoreType)bufferingType defer:(BOOL)deferCreation {
	self = [super initWithContentRect:contentRect styleMask:windowStyle backing:bufferingType defer:deferCreation];
	if(self) {
		[self setShowsResizeIndicator:NO];
		[self setExcludedFromWindowsMenu:YES];
		[[self standardWindowButton:NSWindowZoomButton] setEnabled:NO];

		// Disallow height resize.
		[self setContentMinSize:NSMakeSize(325, 1)];
		[self setContentMaxSize:NSMakeSize(CGFLOAT_MAX, 1)];
		[self setCollectionBehavior:NSWindowCollectionBehaviorFullScreenAuxiliary];

		[self setFrameUsingName:@"Mini Window"];
	}

	return self;
}

- (void)awakeFromNib {
	[super awakeFromNib];

	[self registerForDraggedTypes:@[NSPasteboardTypeFileURL, iTunesDropType]];

	if([[NSUserDefaults standardUserDefaults] boolForKey:@"miniMode"]) {
		showSentryConsent(self);
	}
}

- (void)toggleToolbarShown:(id)sender {
	// Mini window IS the toolbar, no point in hiding it.
	// Do nothing!
}

- (void)keyDown:(NSEvent *)event {
	BOOL modifiersUsed = ([event modifierFlags] & (NSEventModifierFlagShift |
	                                               NSEventModifierFlagControl |
	                                               NSEventModifierFlagOption |
	                                               NSEventModifierFlagCommand)) ?
	                     YES :
                         NO;
	if(modifiersUsed) {
		[super keyDown:event];
		return;
	}

	switch([event keyCode]) {
		case kVK_Space:
			[playbackController playPauseResume:self];
			break;

		case kVK_Return:
			[playbackController play:self];
			break;

		case kVK_LeftArrow:
			[playbackController eventSeekBackward:self];
			break;

		case kVK_RightArrow:
			[playbackController eventSeekForward:self];
			break;

		case kVK_UpArrow:
			[playbackController volumeUp:self];
			break;

		case kVK_DownArrow:
			[playbackController volumeDown:self];
			break;

		default:
			[super keyDown:event];
			break;
	}
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender {
	NSPasteboard *pboard = [sender draggingPasteboard];
	NSDragOperation sourceDragMask = [sender draggingSourceOperationMask];

	if([[pboard types] containsObject:iTunesDropType]) {
		if(sourceDragMask & NSDragOperationGeneric) {
			return NSDragOperationGeneric;
		}
	}

	if([[pboard types] containsObject:NSPasteboardTypeFileURL]) {
		if(sourceDragMask & NSDragOperationGeneric) {
			return NSDragOperationGeneric;
		}
	}

	return NSDragOperationNone;
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender {
	NSPasteboard *pboard = [sender draggingPasteboard];
	NSDragOperation sourceDragMask = [sender draggingSourceOperationMask];

	if([[pboard types] containsObject:iTunesDropType]) {
		if(sourceDragMask & NSDragOperationGeneric) {
			return NSDragOperationGeneric;
		}
	}

	if([[pboard types] containsObject:NSPasteboardTypeFileURL]) {
		if(sourceDragMask & NSDragOperationGeneric) {
			return NSDragOperationGeneric;
		}
	}

	return NSDragOperationNone;
}

- (BOOL)prepareForDragOperation:(id<NSDraggingInfo>)sender {
	NSPasteboard *pboard = [sender draggingPasteboard];
	NSDragOperation sourceDragMask = [sender draggingSourceOperationMask];

	if([[pboard types] containsObject:iTunesDropType]) {
		if(sourceDragMask & NSDragOperationGeneric) {
			return YES;
		}
	}

	if([[pboard types] containsObject:NSPasteboardTypeFileURL]) {
		if(sourceDragMask & NSDragOperationGeneric) {
			return YES;
		}
	}

	return NO;
}

- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender {
	NSPasteboard *pboard = [sender draggingPasteboard];

	if([[pboard types] containsObject:iTunesDropType] ||
	   [[pboard types] containsObject:NSPasteboardTypeFileURL]) {
		NSUInteger row = [[playlistController arrangedObjects] count];
		return [playlistController tableView:[playlistController tableView] acceptDrop:sender row:row dropOperation:NSTableViewDropOn];
	}

	return NO;
}

@end
