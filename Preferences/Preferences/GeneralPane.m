//
//  GeneralPane.m
//  Preferences
//
//  Created by Christopher Snowhill on 6/20/22.
//

#import "GeneralPane.h"

#import "PathSuggester.h"

@implementation GeneralPane

- (NSString *)title {
	return NSLocalizedPrefString(@"General");
}

- (NSImage *)icon {
	if(@available(macOS 11.0, *))
		return [NSImage imageWithSystemSymbolName:@"gearshape.fill" accessibilityDescription:nil];
	return [[NSImage alloc] initWithContentsOfFile:[[NSBundle bundleForClass:[self class]] pathForImageResource:@"general"]];
}

- (IBAction)addPath:(id)sender {
	NSOpenPanel *panel = [NSOpenPanel openPanel];
	[panel setAllowsMultipleSelection:NO];
	[panel setCanChooseDirectories:YES];
	[panel setCanChooseFiles:NO];
	[panel setFloatingPanel:YES];
	NSInteger result = [panel runModal];
	if(result == NSModalResponseOK) {
		[sandboxPathBehaviorController addUrl:[panel URL]];
	}
}

- (IBAction)deleteSelectedPaths:(id)sender {
	NSArray *selectedObjects = [sandboxPathBehaviorController selectedObjects];
	if(selectedObjects && [selectedObjects count]) {
		NSArray *tokens = [selectedObjects valueForKey:@"token"];
		for(id token in tokens) {
			[sandboxPathBehaviorController removeToken:token];
		}
	}
}

- (IBAction)removeStaleEntries:(id)sender {
	[sandboxPathBehaviorController removeStaleEntries];
}

- (NSView *_Nullable)tableView:(NSTableView *)tableView viewForTableColumn:(NSTableColumn *_Nullable)tableColumn row:(NSInteger)row {
	NSString *cellIdentifier = @"";
	NSTextAlignment cellTextAlignment = NSTextAlignmentLeft;

	NSDictionary *item = [[sandboxPathBehaviorController arrangedObjects] objectAtIndex:row];

	/*float fontSize = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] floatForKey:@"fontSize"];*/

	NSString *cellText = @"";

	if(item) {
		cellIdentifier = [tableColumn identifier];
		if([cellIdentifier isEqualToString:@"path"]) {
			cellText = [item objectForKey:@"path"];
		} else if([cellIdentifier isEqualToString:@"valid"]) {
			cellText = [item objectForKey:@"valid"];
		}
	}

	NSView *view = [tableView makeViewWithIdentifier:cellIdentifier owner:nil];
	if(view && [view isKindOfClass:[NSTableCellView class]]) {
		NSTableCellView *cellView = (NSTableCellView *)view;

		if(cellView.textField) {
			cellView.textField.allowsDefaultTighteningForTruncation = YES;

			//NSFont *font = [NSFont monospacedDigitSystemFontOfSize:fontSize weight:NSFontWeightRegular];

			//cellView.textField.font = font;
			cellView.textField.stringValue = cellText;
			cellView.textField.alignment = cellTextAlignment;

			if(cellView.textField.intrinsicContentSize.width > cellView.textField.frame.size.width - 4) {
				cellView.textField.toolTip = cellText;
			}
		}
	}

	return view;
}

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem {
	SEL action = [menuItem action];

	if(action == @selector(addPath:) ||
	   action == @selector(deleteSelectedPaths:) ||
	   action == @selector(removeStaleEntries:) ||
	   action == @selector(showPathSuggester:)) {
		return YES;
	}

	return NO;
}

- (IBAction)showPathSuggester:(id)sender {
	[pathSuggester beginSuggestion:sender];
}

@end
