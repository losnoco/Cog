//
//  PreferencesController.m
//  Preferences
//
//  Created by Vincent Spader on 9/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "GeneralPreferencesPlugin.h"

@implementation GeneralPreferencesPlugin

+ (NSArray *)preferencePanes
{
	GeneralPreferencesPlugin *plugin = [[GeneralPreferencesPlugin alloc] init];
	[NSBundle loadNibNamed:@"Preferences" owner:plugin];
	
	return [NSArray arrayWithObjects: 
			[plugin playlistPane],
			[plugin hotKeyPane],
			[plugin remotePane],
			[plugin updatesPane],
			[plugin outputPane],
			[plugin scrobblerPane],
			nil];
}	

- (HotKeyPane *)hotKeyPane
{
	return hotKeyPane;
}

- (OutputPane *)outputPane
{
	return outputPane;
}

- (GeneralPreferencePane *)remotePane
{
	return [GeneralPreferencePane preferencePaneWithView:remoteView title:NSLocalizedStringFromTableInBundle(@"Remote", nil, [NSBundle bundleForClass:[self class]],  @"")  iconNamed:@"apple_remote"];
}

- (GeneralPreferencePane *)updatesPane
{
	return [GeneralPreferencePane preferencePaneWithView:updatesView title:NSLocalizedStringFromTableInBundle(@"Updates", nil, [NSBundle bundleForClass:[self class]], @"")  iconNamed:@"updates"];
}

- (GeneralPreferencePane *)scrobblerPane
{
	return [GeneralPreferencePane preferencePaneWithView:scrobblerView title:NSLocalizedStringFromTableInBundle(@"Last.fm", nil, [NSBundle bundleForClass:[self class]], @"")  iconNamed:@"lastfm"];
}

- (GeneralPreferencePane *)playlistPane
{
	return [GeneralPreferencePane preferencePaneWithView:playlistView title:NSLocalizedStringFromTableInBundle(@"Playlist", nil, [NSBundle bundleForClass:[self class]], @"")  iconNamed:@"playlist"];
}

@end
