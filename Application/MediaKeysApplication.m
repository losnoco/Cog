//
//  MediaKeysApplication.m
//  Cog
//
//  Created by Vincent Spader on 10/3/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "MediaKeysApplication.h"
#import "AppController.h"
#import "Logging.h"

#import <MediaPlayer/MPMediaItem.h>
#import <MediaPlayer/MPNowPlayingInfoCenter.h>
#import <MediaPlayer/MPRemoteCommand.h>
#import <MediaPlayer/MPRemoteCommandCenter.h>
#import <MediaPlayer/MPRemoteCommandEvent.h>

@import Firebase;

@implementation MediaKeysApplication {
	AppController *_appController;
}

- (void)finishLaunching {
	[super finishLaunching];
	_appController = (AppController *)[self delegate];

	[[NSUserDefaults standardUserDefaults] registerDefaults:@{@"NSApplicationCrashOnExceptions" : @(YES)}];
	[FIRApp configure];

	MPRemoteCommandCenter *remoteCommandCenter = [MPRemoteCommandCenter sharedCommandCenter];

	[remoteCommandCenter.playCommand setEnabled:YES];
	[remoteCommandCenter.pauseCommand setEnabled:YES];
	[remoteCommandCenter.togglePlayPauseCommand setEnabled:YES];
	[remoteCommandCenter.stopCommand setEnabled:YES];
	[remoteCommandCenter.changePlaybackPositionCommand setEnabled:YES];
	[remoteCommandCenter.nextTrackCommand setEnabled:YES];
	[remoteCommandCenter.previousTrackCommand setEnabled:YES];

	[[remoteCommandCenter playCommand] addTarget:self action:@selector(clickPlay)];
	[[remoteCommandCenter pauseCommand] addTarget:self action:@selector(clickPause)];
	[[remoteCommandCenter togglePlayPauseCommand] addTarget:self action:@selector(clickPlay)];
	[[remoteCommandCenter stopCommand] addTarget:self action:@selector(clickStop)];
	[[remoteCommandCenter changePlaybackPositionCommand] addTarget:self action:@selector(clickSeek:)];
	[[remoteCommandCenter nextTrackCommand] addTarget:self action:@selector(clickNext)];
	[[remoteCommandCenter previousTrackCommand] addTarget:self action:@selector(clickPrev)];
}

- (MPRemoteCommandHandlerStatus)clickPlay {
	[_appController clickPlay];
	return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)clickPause {
	[_appController clickPause];
	return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)clickStop {
	[_appController clickStop];
	return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)clickNext {
	[_appController clickNext];
	return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)clickPrev {
	[_appController clickPrev];
	return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)clickSeek:(MPChangePlaybackPositionCommandEvent *)event {
	[_appController clickSeek:event.positionTime];
	return MPRemoteCommandHandlerStatusSuccess;
}

@end
