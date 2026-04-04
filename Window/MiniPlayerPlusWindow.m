//
//  MiniPlayerPlusWindow.m
//  Cog
//

#import "MiniPlayerPlusWindow.h"

#import <Carbon/Carbon.h>

extern NSString *iTunesDropType;

@implementation MiniPlayerPlusWindow

- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSWindowStyleMask)windowStyle backing:(NSBackingStoreType)bufferingType defer:(BOOL)deferCreation {
	self = [super initWithContentRect:contentRect styleMask:windowStyle backing:bufferingType defer:deferCreation];
	if(self) {
		[self setShowsResizeIndicator:YES];
		[self setExcludedFromWindowsMenu:YES];
		[[self standardWindowButton:NSWindowZoomButton] setEnabled:NO];

		[self setContentMinSize:NSMakeSize(240, 200)];
		[self setCollectionBehavior:NSWindowCollectionBehaviorFullScreenAuxiliary];

		[self setFrameUsingName:@"Mini Plus Window"];
	}

	return self;
}

- (void)awakeFromNib {
	[super awakeFromNib];

	[self registerForDraggedTypes:@[NSPasteboardTypeFileURL, iTunesDropType]];
}

- (void)toggleToolbarShown:(id)sender {
	// Mini plus window has a toolbar with controls; don't hide it.
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
