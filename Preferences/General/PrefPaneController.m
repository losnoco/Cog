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

	return [NSArray arrayWithObjects: [prefPaneController hotKeyPane], [prefPaneController fileDrawerPane], [prefPaneController remotePane], [prefPaneController updatesPane], [prefPaneController outputPane], nil];
}	

- (HotKeyPane *)hotKeyPane
{
	return hotKeyPane;
}

- (FileDrawerPane *)fileDrawerPane
{
	return fileDrawerPane;
}

- (RemotePane *)remotePane
{
	return remotePane;
}

- (UpdatesPane *)updatesPane
{
	return updatesPane;
}

- (OutputPane *)outputPane
{
	return outputPane;
}

@end
