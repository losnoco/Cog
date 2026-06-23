//
//  MainSplitViewController.m
//  Cog
//
//  Created by Kevin López Brante on 6/10/26.
//

#import "MainSplitViewController.h"

static void *kMainSplitViewControllerContext = &kMainSplitViewControllerContext;

@implementation MainSplitViewController {
	NSSplitViewItem *sidebarItem;
}

- (void)setupWithSidebarViewController:(NSViewController *)sidebarViewController
                 contentViewController:(NSViewController *)contentViewController {
	self.splitView.vertical = YES;
	self.splitView.dividerStyle = NSSplitViewDividerStyleThin;

	sidebarItem = [NSSplitViewItem sidebarWithViewController:sidebarViewController];
	sidebarItem.minimumThickness = 150;
	sidebarItem.maximumThickness = 400;
	sidebarItem.canCollapse = YES;
	sidebarItem.holdingPriority = NSLayoutPriorityDefaultLow + 10;
	sidebarItem.allowsFullHeightLayout = YES;
	sidebarItem.titlebarSeparatorStyle = NSTitlebarSeparatorStyleAutomatic;

	NSSplitViewItem *contentItem = [NSSplitViewItem splitViewItemWithViewController:contentViewController];
	contentItem.minimumThickness = 400;
	contentItem.holdingPriority = NSLayoutPriorityDefaultLow;

	[self addSplitViewItem:sidebarItem];
	[self addSplitViewItem:contentItem];

	self.splitView.autosaveName = @"MainWindowSplitView";

	// Visibility is owned by the FileTreeShowSideView default, not the split view autosave
	sidebarItem.collapsed = ![[NSUserDefaults standardUserDefaults] boolForKey:@"FileTreeShowSideView"];

	[sidebarItem addObserver:self
	              forKeyPath:@"collapsed"
	                 options:0
	                 context:kMainSplitViewControllerContext];
}

- (void)dealloc {
	[sidebarItem removeObserver:self
	                 forKeyPath:@"collapsed"
	                    context:kMainSplitViewControllerContext];
}

- (NSSplitViewItem *)sidebarItem {
	return sidebarItem;
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
	if(context != kMainSplitViewControllerContext) {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
		return;
	}

	BOOL collapsed = sidebarItem.isCollapsed;
	[[NSUserDefaults standardUserDefaults] setBool:!collapsed forKey:@"FileTreeShowSideView"];

	NSWindow *window = self.splitView.window;
	if(window) {
		NSResponder *responder = collapsed ? self.contentFirstResponder : self.sidebarFirstResponder;
		if(responder) {
			[window makeFirstResponder:responder];
		}
	}
}

@end
