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

@implementation MediaKeysApplication {
	AppController *_appController;
}

- (void)finishLaunching {
	[super finishLaunching];
	_appController = (AppController *)[self delegate];

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
	dispatch_async(dispatch_get_main_queue(), ^{
		[self->_appController clickPlay];
	});
	return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)clickPause {
	dispatch_async(dispatch_get_main_queue(), ^{
		[self->_appController clickPause];
	});
	return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)clickStop {
	dispatch_async(dispatch_get_main_queue(), ^{
		[self->_appController clickStop];
	});
	return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)clickNext {
	dispatch_async(dispatch_get_main_queue(), ^{
		[self->_appController clickNext];
	});
	return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)clickPrev {
	dispatch_async(dispatch_get_main_queue(), ^{
		[self->_appController clickPrev];
	});
	return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)clickSeek:(MPChangePlaybackPositionCommandEvent *)event {
	NSTimeInterval positionTime = event.positionTime;
	dispatch_async(dispatch_get_main_queue(), ^{
		[self->_appController clickSeek:positionTime];
	});
	return MPRemoteCommandHandlerStatusSuccess;
}

@end
