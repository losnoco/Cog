//
//  MainWindow.h
//  Cog
//
//  Created by Vincent Spader on 2/22/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface MainWindow : NSWindow {
	IBOutlet NSView *playlistView;
	IBOutlet NSToolbar *mainToolbar;
	IBOutlet NSTextField *outputFormatField;
	IBOutlet NSSearchField *searchField;
	// The SignalInspectorWindowController, opened by clicking the output status.
	IBOutlet id signalInspector;
	// The bar under the playlist, with the total duration and the output status.
	IBOutlet NSView *statusBar;
}

- (IBAction)openSearch:(id)sender;
- (IBAction)toggleStatusBar:(id)sender;

@end
