//
//  MediaKeysApplication.m
//  Cog
//
//  Created by Vincent Spader on 10/3/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "MediaKeysApplication.h"
#import "AppController.h"
#import "SPMediaKeyTap.h"
#import "Logging.h"

#import <MediaPlayer/MPNowPlayingInfoCenter.h>
#import <MediaPlayer/MPRemoteCommandCenter.h>
#import <MediaPlayer/MPRemoteCommand.h>
#import <MediaPlayer/MPMediaItem.h>
#import <MediaPlayer/MPRemoteCommandEvent.h>

@implementation MediaKeysApplication

+(void)initialize;
{
    if([self class] != [MediaKeysApplication class]) return;

    // Register defaults for the whitelist of apps that want to use media keys
    [[NSUserDefaults standardUserDefaults] registerDefaults:[NSDictionary dictionaryWithObjectsAndKeys:
            [SPMediaKeyTap defaultMediaKeyUserBundleIdentifiers], kMediaKeyUsingBundleIdentifiersDefaultsKey,
            nil]];
}

- (void)finishLaunching {
    [super finishLaunching];
    
    [[NSUserDefaults standardUserDefaults] addObserver:self
                                            forKeyPath:@"allowLastfmMediaKeys"
                                               options:NSKeyValueObservingOptionNew
                                               context:nil];
    
    if (NSClassFromString(@"MPRemoteCommandCenter")) {
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
    } else {
        keyTap = [[SPMediaKeyTap alloc] initWithDelegate:self];
        if([SPMediaKeyTap usesGlobalMediaKeyTap]) {
            [keyTap startWatchingMediaKeys];
        }
        else
            ALog(@"Media key monitoring disabled");
    }
}

- (MPRemoteCommandHandlerStatus)clickPlay {
    [(AppController *)[self delegate] clickPlay];
    return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)clickPause {
    [(AppController *)[self delegate] clickPause];
    return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)clickStop {
    [(AppController *)[self delegate] clickStop];
    return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)clickNext {
    [(AppController *)[self delegate] clickNext];
    return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)clickPrev {
    [(AppController *)[self delegate] clickPrev];
    return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)clickSeek: (MPChangePlaybackPositionCommandEvent*)event {
    [(AppController *)[self delegate] clickSeek:event.positionTime];
    return MPRemoteCommandHandlerStatusSuccess;
}

- (void)sendEvent: (NSEvent*)event
{
    BOOL shouldHandleMediaKeyEventLocally = ![SPMediaKeyTap usesGlobalMediaKeyTap];

	if(shouldHandleMediaKeyEventLocally && [event type] == NSSystemDefined && [event subtype] == 8 )
	{
		[self mediaKeyTap:nil receivedMediaKeyEvent:event];
	}

	[super sendEvent: event];
}

-(void)mediaKeyTap:(SPMediaKeyTap*)keyTap receivedMediaKeyEvent:(NSEvent*)event;
{
    NSAssert([event type] == NSSystemDefined && [event subtype] == SPSystemDefinedEventMediaKeys, @"Unexpected NSEvent in mediaKeyTap:receivedMediaKeyEvent:");
    
    int keyCode = (([event data1] & 0xFFFF0000) >> 16);
    int keyFlags = ([event data1] & 0x0000FFFF);
    BOOL keyIsPressed = (((keyFlags & 0xFF00) >> 8)) == 0xA;
    
    if (!keyIsPressed) // pressed and released
    {
        switch( keyCode )
        {
            case NX_KEYTYPE_PLAY:
                [(AppController *)[self delegate] clickPlay];
                break;
                
            case NX_KEYTYPE_NEXT:
            case NX_KEYTYPE_FAST:
                [(AppController *)[self delegate] clickNext];
                break;
                
            case NX_KEYTYPE_PREVIOUS:
            case NX_KEYTYPE_REWIND:
                [(AppController *)[self delegate] clickPrev];
                break;
        }
    }
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
    if ([keyPath isEqualToString:@"allowLastfmMediaKeys"])
    {
        NSUserDefaults *defs = [NSUserDefaults standardUserDefaults];
        BOOL allowLastfmMediaKeys = [defs boolForKey:@"allowLastfmMediaKeys"];
        NSArray *old = [defs arrayForKey:kMediaKeyUsingBundleIdentifiersDefaultsKey];
        
        NSMutableArray *new = [old mutableCopy];
        NSArray *lastfmIds = [NSArray arrayWithObjects:@"fm.last.Last.fm", @"fm.last.Scrobbler", nil];
        if (allowLastfmMediaKeys)
        {
            [new addObjectsFromArray:lastfmIds];
        }
        else
        {
            [new removeObjectsInArray:lastfmIds];
        }
        
        [defs setObject:new forKey:kMediaKeyUsingBundleIdentifiersDefaultsKey];
    }
    else
    {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}


@end
