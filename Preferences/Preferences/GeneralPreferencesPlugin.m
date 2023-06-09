//
//  PreferencesController.m
//  Preferences
//
//  Created by Vincent Spader on 9/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "GeneralPreferencesPlugin.h"
#import "ColorToValueTransformer.h"
#import "PathToFileTransformer.h"
#import "TimeIntervalToStringTransformer.h"

@implementation GeneralPreferencesPlugin

+ (void)initialize {
	NSValueTransformer *pathToFileTransformer = [[PathToFileTransformer alloc] init];
	[NSValueTransformer setValueTransformer:pathToFileTransformer
	                                forName:@"PathToFileTransformer"];

	NSValueTransformer *colorToValueTransformer = [[ColorToValueTransformer alloc] init];
	[NSValueTransformer setValueTransformer:colorToValueTransformer
	                                forName:@"ColorToValueTransformer"];

	NSValueTransformer *timeIntervalToStringTransformer = [[TimeIntervalToStringTransformer alloc] init];
	[NSValueTransformer setValueTransformer:timeIntervalToStringTransformer
	                                forName:@"TimeIntervalToStringTransformer"];
}

+ (NSArray *)preferencePanes {
	GeneralPreferencesPlugin *plugin = [[GeneralPreferencesPlugin alloc] init];
	[[NSBundle bundleWithIdentifier:@"org.cogx.cog.preferences"] loadNibNamed:@"Preferences"
	                                                                    owner:plugin
	                                                          topLevelObjects:nil];

	return @[[plugin playlistPane],
		     [plugin hotKeyPane],
		     [plugin updatesPane],
		     [plugin outputPane],
		     [plugin generalPane],
		     [plugin notificationsPane],
		     [plugin appearancePane],
		     [plugin midiPane]];
}

- (HotKeyPane *)hotKeyPane {
	return hotKeyPane;
}

- (OutputPane *)outputPane {
	return outputPane;
}

- (MIDIPane *)midiPane {
	return midiPane;
}

- (GeneralPane *)generalPane {
	return generalPane;
}

- (GeneralPreferencePane *)updatesPane {
	return [GeneralPreferencePane preferencePaneWithView:updatesView
	                                               title:NSLocalizedPrefString(@"Updates")
	                                      systemIconName:@"arrow.triangle.2.circlepath.circle.fill"
	                                      orOldIconNamed:@"updates"];
}

- (GeneralPreferencePane *)playlistPane {
	return [GeneralPreferencePane preferencePaneWithView:playlistView
	                                               title:NSLocalizedPrefString(@"Playlist")
	                                      systemIconName:@"music.note.list"
	                                      orOldIconNamed:@"playlist"];
}

- (GeneralPreferencePane *)notificationsPane {
	if(iTunesStyleCheck) {
		iTunesStyleCheck.hidden = YES;
		NSSize size = notificationsView.frame.size;
		size.height -= 18;
		[notificationsView setFrameSize:size];
	}

	return [GeneralPreferencePane preferencePaneWithView:notificationsView
	                                               title:NSLocalizedPrefString(@"Notifications")
	                                      systemIconName:@"bell.fill"
	                                      orOldIconNamed:@"growl"];
}

- (GeneralPreferencePane *)appearancePane {
	return [GeneralPreferencePane preferencePaneWithView:appearanceView
	                                               title:NSLocalizedPrefString(@"Appearance")
	                                      systemIconName:@"paintpalette.fill"
	                                      orOldIconNamed:@"appearance"];
}

@end
