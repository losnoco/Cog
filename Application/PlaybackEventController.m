//
//  PlaybackEventController.m
//  Cog
//
//  Created by Vincent Spader on 3/5/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//
//  New Notification Center code shamelessly based off this:
//  https://github.com/kbhomes/radiant-player-mac/tree/master/radiant-player-mac/Notifications

#import "PlaybackEventController.h"

#import "AudioScrobbler.h"

NSString *TrackPlaying = @"org.cogx.Cog-Track-Playing";
NSString *TrackStopped = @"org.cogx.Cog-Track-Stopped";
NSString *TrackPaused = @"org.cogx.Cog-Track-Paused";

NSString *TrackArtist = @"artist";
NSString *TrackAlbum = @"album";
NSString *TrackTitle = @"title";
NSString *TrackGenre = @"genre";
NSString *TrackComposer = @"composer";
NSString *TrackNumber = @"trackNumber";
NSString *TrackLength = @"length";
NSString *TrackCurrentTime = @"currentTime";

@implementation PlaybackEventController

- (void)initDefaults
{
	NSDictionary *defaultsDictionary = [NSDictionary dictionaryWithObjectsAndKeys:
										[NSNumber numberWithBool:YES], @"enableAudioScrobbler",
										[NSNumber numberWithBool:NO],  @"automaticallyLaunchLastFM",
                                        [NSNumber numberWithBool:YES], @"notifications.enable",
                                        [NSNumber numberWithBool:NO], @"notifications.use-growl",
                                        [NSNumber numberWithBool:YES], @"notifications.itunes-style",
                                        [NSNumber numberWithBool:YES], @"notifications.show-album-art",
										nil];
	
	[[NSUserDefaults standardUserDefaults] registerDefaults:defaultsDictionary];
}

- (id)init
{
	self = [super init];
	if (self)
	{
		[self initDefaults];
		
		queue = [[NSOperationQueue alloc] init];
		[queue setMaxConcurrentOperationCount:1];
		
		scrobbler = [[AudioScrobbler alloc] init];
        [[NSUserNotificationCenter defaultUserNotificationCenter] setDelegate:self];
		[GrowlApplicationBridge setGrowlDelegate:self];
        
        entry = nil;
	}
	
	return self;
}

- (void)dealloc
{
	[queue release];
    
    [entry release];
	
	[super dealloc];
}

- (NSDictionary *)fillNotificationDictionary:(PlaylistEntry *)pe
{
    NSMutableDictionary *dict = [[NSMutableDictionary alloc] init];
    
    if ([pe title]) [dict setObject:[pe title] forKey:TrackTitle];
    if ([pe artist]) [dict setObject:[pe artist] forKey:TrackArtist];
    if ([pe album]) [dict setObject:[pe album] forKey:TrackAlbum];
    if ([pe genre]) [dict setObject:[pe genre] forKey:TrackGenre];
    if ([pe track]) [dict setObject:[pe track] forKey:TrackNumber];
    if ([pe lengthText]) [dict setObject:[pe lengthText] forKey:TrackLength];
    if ([pe positionText]) [dict setObject:[pe positionText] forKey:TrackCurrentTime];
    
    return dict;
}

- (void)performPlaybackDidBeginActions:(PlaylistEntry *)pe
{
    if (NO == [pe error]) {
        [entry release];
        entry = [pe retain];
        
        [[NSDistributedNotificationCenter defaultCenter] postNotificationName:TrackPlaying object:nil userInfo:[self fillNotificationDictionary:pe]];
        
        NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

        if ([defaults boolForKey:@"notifications.enable"]) {
            if([defaults boolForKey:@"enableAudioScrobbler"]) {
                [scrobbler start:pe];
                if ([AudioScrobbler isRunning]) return;
            }

            if ([defaults boolForKey:@"notifications.use-growl"]) {
                // Note: We don't want to send a growl notification on resume.
                [GrowlApplicationBridge notifyWithTitle:[pe title]
                                            description:[pe artist]
                                       notificationName:@"Stream Changed"
                                               iconData:[pe albumArtInternal]
                                               priority:0
                                               isSticky:NO
                                           clickContext:nil];
            }
            else {
                NSUserNotification *notif = [[NSUserNotification alloc] init];
                notif.title = [pe title];
                
                if ([defaults boolForKey:@"notifications.itunes-style"]) {
                    notif.subtitle = [NSString stringWithFormat:@"%@ - %@", [pe artist], [pe album]];
                    [notif setValue:@YES forKey:@"_showsButtons"];
                }
                else {
                    notif.informativeText = [NSString stringWithFormat:@"%@ - %@", [pe artist], [pe album]];
                }
                
                if ([notif respondsToSelector:@selector(setContentImage:)]) {
                    if ([defaults boolForKey:@"notifications.show-album-art"] && [pe albumArtInternal]) {
                        NSImage *image = [pe albumArt];
                        
                        if ([defaults boolForKey:@"notifications.itunes-style"]) {
                            [notif setValue:image forKey:@"_identityImage"];
                        }
                        else {
                            notif.contentImage = image;
                        }
                    }
                }
                
                notif.actionButtonTitle = @"Skip";
                
                [[NSUserNotificationCenter defaultUserNotificationCenter] scheduleNotification:notif];
            }
        }
	}
}

- (void)performPlaybackDidPauseActions
{
    [[NSDistributedNotificationCenter defaultCenter] postNotificationName:TrackPaused object:nil userInfo:[self fillNotificationDictionary:entry]];
	if([[NSUserDefaults standardUserDefaults] boolForKey:@"enableAudioScrobbler"]) {
		[scrobbler pause];
	}
}

- (void)performPlaybackDidResumeActions
{
    [[NSDistributedNotificationCenter defaultCenter] postNotificationName:TrackPlaying object:nil userInfo:[self fillNotificationDictionary:entry]];
	if([[NSUserDefaults standardUserDefaults] boolForKey:@"enableAudioScrobbler"]) {
		[scrobbler resume];
	}
}

- (void)performPlaybackDidStopActions
{
    [[NSDistributedNotificationCenter defaultCenter] postNotificationName:TrackStopped object:nil userInfo:[self fillNotificationDictionary:entry]];
    [entry release];
    entry = nil;
	if([[NSUserDefaults standardUserDefaults] boolForKey:@"enableAudioScrobbler"]) {
		[scrobbler stop];
	}
}


- (void)awakeFromNib
{
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(playbackDidBegin:) name:CogPlaybackDidBeginNotficiation object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(playbackDidPause:) name:CogPlaybackDidPauseNotficiation object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(playbackDidResume:) name:CogPlaybackDidResumeNotficiation object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(playbackDidStop:)  name:CogPlaybackDidStopNotficiation object:nil];
    
    [[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.enableGrowlMist"		options:0 context:nil];
    
    [self toggleGrowlMist];
}

- (void) toggleGrowlMist
{
    BOOL enableMist = [[NSUserDefaults standardUserDefaults] boolForKey:@"enableGrowlMist"];
    [GrowlApplicationBridge setShouldUseBuiltInNotifications:enableMist];
}

- (void) observeValueForKeyPath:(NSString *)keyPath
					   ofObject:(id)object
						 change:(NSDictionary *)change
                        context:(void *)context
{
	if ([keyPath isEqualToString:@"values.enableGrowlMist"]) {
        [self toggleGrowlMist];
	}
}

- (void)playbackDidBegin:(NSNotification *)notification
{
	NSOperation *op = [[NSInvocationOperation alloc] initWithTarget:self selector:@selector(performPlaybackDidBeginActions:) object:[notification object]];
	[queue addOperation:op];
	[op release];
}

- (void)playbackDidPause:(NSNotification *)notification
{
	NSOperation *op = [[NSInvocationOperation alloc] initWithTarget:self selector:@selector(performPlaybackDidPauseActions) object:nil];
	[queue addOperation:op];
	[op release];
}

- (void)playbackDidResume:(NSNotification *)notification
{
	NSOperation *op = [[NSInvocationOperation alloc] initWithTarget:self selector:@selector(performPlaybackDidResumeActions) object:nil];
	[queue addOperation:op];
	[op release];
}

- (void)playbackDidStop:(NSNotification *)notification
{
	NSOperation *op = [[NSInvocationOperation alloc] initWithTarget:self selector:@selector(performPlaybackDidStopActions) object:nil];
	[queue addOperation:op];
	[op release];
}

- (NSDictionary *) registrationDictionaryForGrowl
{
	NSArray *notifications = [NSArray arrayWithObjects:@"Stream Changed", nil];
	
	return [NSDictionary dictionaryWithObjectsAndKeys:
			@"Cog", GROWL_APP_NAME,  
			notifications, GROWL_NOTIFICATIONS_ALL, 
			notifications, GROWL_NOTIFICATIONS_DEFAULT,
			nil];
}

- (void)userNotificationCenter:(NSUserNotificationCenter *)center didActivateNotification:(NSUserNotification *)notification
{
    switch (notification.activationType)
    {
        case NSUserNotificationActivationTypeActionButtonClicked:
            [playbackController next:self];
            break;
            
        case NSUserNotificationActivationTypeContentsClicked:
        {
            NSWindow *window = [[NSUserDefaults standardUserDefaults] boolForKey:@"miniMode"] ? miniWindow : mainWindow;
            
            [NSApp activateIgnoringOtherApps:YES];
            [window makeKeyAndOrderFront:self];
        };
            break;
            
        default:
            break;
    }
}

@end
