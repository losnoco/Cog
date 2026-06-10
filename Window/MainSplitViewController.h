//
//  MainSplitViewController.h
//  Cog
//
//  Created by Kevin López Brante on 6/10/26.
//

#import <Cocoa/Cocoa.h>

@interface MainSplitViewController : NSSplitViewController

// Responders to focus when the sidebar is revealed or hidden, respectively.
@property(nonatomic, weak) NSResponder *sidebarFirstResponder;
@property(nonatomic, weak) NSResponder *contentFirstResponder;

@property(nonatomic, readonly) NSSplitViewItem *sidebarItem;

- (void)setupWithSidebarViewController:(NSViewController *)sidebarViewController
                 contentViewController:(NSViewController *)contentViewController;

@end
