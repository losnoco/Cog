//
//  PreferencesController.m
//  Preferences
//
//  Created by Vincent Spader on 9/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "PrefPaneController.h"
#import "SS_PreferencePaneProtocol.h"

@implementation PrefPaneController

+ (NSArray *)preferencePanes
{
	BOOL loaded;
	
	PrefPaneController *prefPaneController = [[PrefPaneController alloc] init];
	loaded = [NSBundle loadNibNamed:@"Preferences" owner:prefPaneController];

	return [NSArray arrayWithObjects: [prefPaneController hotKeyPane], [prefPaneController fileDrawerPane], [prefPaneController remotePane], [prefPaneController updatesPane], [prefPaneController outputPane], [prefPaneController scrobblerPane], nil];
}	

- (HotKeyPane *)hotKeyPane
{
	return hotKeyPane;
}

- (FileDrawerPane *)fileDrawerPane
{
	return fileDrawerPane;
}

- (OutputPane *)outputPane
{
	return outputPane;
}

- (PreferencePane *)remotePane
{
	return [PreferencePane preferencePaneWithView:remoteView name:@"Remote" icon:@"apple_remote"];
}

- (PreferencePane *)updatesPane
{
	return [PreferencePane preferencePaneWithView:updatesView name:@"Updates" icon:@"updates"];
}

- (PreferencePane *)scrobblerPane
{
	return [PreferencePane preferencePaneWithView:scrobblerView name:@"Last.fm" icon:@"lastfm"];
}

@end
