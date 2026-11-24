//
//  PlaybackEventController.m
//  Cog
//
//  Created by Vincent Spader on 3/5/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.

#import "PlaybackEventController.h"

#import "PlaylistEntry.h"

@implementation PlaybackEventController {
	NSOperationQueue *queue;

	PlaylistEntry *entry;

	Boolean didGainUN;
}

- (void)initDefaults {
	NSDictionary *defaultsDictionary = @{
		@"enableAudioScrobbler": @YES,
		@"enableListenBrainz": @NO,
		@"listenBrainzUrl": @"https://api.listenbrainz.org",
		@"notifications.enable": @YES,
		@"notifications.itunes-style": @YES,
		@"notifications.show-album-art": @YES
	};

	[[NSUserDefaults standardUserDefaults] registerDefaults:defaultsDictionary];
}

- (id)init {
	self = [super init];
	if(self) {
		[self initDefaults];

		didGainUN = NO;

		{
			UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];
			[center
			requestAuthorizationWithOptions:UNAuthorizationOptionAlert
			              completionHandler:^(BOOL granted, NSError *_Nullable error) {
				              self->didGainUN = granted;

				              if(granted) {
					              UNNotificationAction *skipAction = [UNNotificationAction
					              actionWithIdentifier:@"skip"
												 title:NSLocalizedString(@"SkipAction", @"")
					                           options:UNNotificationActionOptionNone];

					              UNNotificationCategory *playCategory = [UNNotificationCategory
					              categoryWithIdentifier:@"play"
					                             actions:@[skipAction]
					                   intentIdentifiers:@[]
					                             options:UNNotificationCategoryOptionNone];

					              [center setNotificationCategories:
					                      [NSSet setWithObject:playCategory]];
				              }
			              }];

			[center setDelegate:self];
		}

		queue = [NSOperationQueue new];
		[queue setMaxConcurrentOperationCount:1];

		entry = nil;
	}

	return self;
}

- (void)userNotificationCenter:(UNUserNotificationCenter *)center
       willPresentNotification:(UNNotification *)notification
         withCompletionHandler:
         (void (^)(UNNotificationPresentationOptions options))completionHandler {
	UNNotificationPresentationOptions presentationOptions = UNNotificationPresentationOptionBanner | UNNotificationPresentationOptionList;

	completionHandler(presentationOptions);
}

- (void)userNotificationCenter:(UNUserNotificationCenter *)center
didReceiveNotificationResponse:(UNNotificationResponse *)response
         withCompletionHandler:(void (^)(void))completionHandler {
	if([[response actionIdentifier] isEqualToString:@"skip"]) {
		[playbackController next:self];
	} else if([[response actionIdentifier] isEqualToString:UNNotificationDefaultActionIdentifier]) {
		dispatch_async(dispatch_get_main_queue(), ^{
			[self showPlayerWindow];
		});
	}
	completionHandler();
}

// Clicking a notification brings forward whichever player window is in use.
- (void)showPlayerWindow {
	NSWindow *window;
	if([[NSUserDefaults standardUserDefaults] boolForKey:@"miniPlusMode"])
		window = miniPlusWindow;
	else if([[NSUserDefaults standardUserDefaults] boolForKey:@"miniMode"])
		window = miniWindow;
	else
		window = mainWindow;

	[NSApp activateIgnoringOtherApps:YES];
	[window makeKeyAndOrderFront:self];
}

- (void)performPlaybackDidBeginActions:(PlaylistEntry *)pe {
	if(NO == [pe error]) {
		entry = pe;

		NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

		if([defaults boolForKey:@"notifications.enable"]) {
			{
				if(didGainUN) {
					UNUserNotificationCenter *center =
					[UNUserNotificationCenter currentNotificationCenter];

					UNMutableNotificationContent *content =
					[UNMutableNotificationContent new];

					content.title = @"Now Playing";

					NSString *subtitle;
					NSString *artist = (pe.artist && [pe.artist length]) ? pe.artist : nil;
					NSString *album = (pe.album && [pe.album length]) ? pe.album : nil;
					if(artist && album) {
						subtitle = [NSString stringWithFormat:@"%@ - %@", artist, album];
					} else if(artist) {
						subtitle = artist;
					} else if(album) {
						subtitle = album;
					} else {
						subtitle = @"";
					}

					NSString *body = [NSString stringWithFormat:@"%@\n%@", [pe title], subtitle];
					content.body = body;
					content.sound = nil;
					content.categoryIdentifier = @"play";

					if([defaults boolForKey:@"notifications.show-album-art"] &&
					   [pe albumArt]) {
						NSError *error = nil;
						NSFileManager *fileManager = [NSFileManager defaultManager];
						NSURL *tmpSubFolderURL = [[NSURL fileURLWithPath:NSTemporaryDirectory()]
						URLByAppendingPathComponent:@"cog-artworks-cache"
						                isDirectory:true];
						if([fileManager createDirectoryAtPath:[tmpSubFolderURL path]
						          withIntermediateDirectories:true
						                           attributes:nil
						                                error:&error]) {
							NSString *tmpFileName =
							[[NSProcessInfo.processInfo globallyUniqueString]
							stringByAppendingString:@".jpg"];
							NSURL *fileURL =
							[tmpSubFolderURL URLByAppendingPathComponent:tmpFileName];
							NSImage *image = [pe albumArt];
							CGImageRef cgRef = [image CGImageForProposedRect:NULL
							                                         context:nil
							                                           hints:nil];

							if(cgRef) {
								NSBitmapImageRep *newRep =
								[[NSBitmapImageRep alloc] initWithCGImage:cgRef];
								NSData *jpgData = [newRep
								representationUsingType:NSBitmapImageFileTypeJPEG
								             properties:@{ NSImageCompressionFactor: @0.5f }];
								[jpgData writeToURL:fileURL atomically:YES];

								UNNotificationAttachment *icon =
								[UNNotificationAttachment attachmentWithIdentifier:@"art"
								                                               URL:fileURL
								                                           options:nil
								                                             error:&error];
								if(error) {
									// We have size limit of 10MB per image attachment.
									NSLog(@"%@", error.localizedDescription);
								} else {
									content.attachments = @[icon];
								}
							}
						}
					}

					UNNotificationRequest *request =
					[UNNotificationRequest requestWithIdentifier:@"PlayTrack"
					                                     content:content
					                                     trigger:nil];

					[center addNotificationRequest:request
					         withCompletionHandler:^(NSError *_Nullable error) {
						         NSLog(@"%@", error.localizedDescription);
					         }];
				}
			}
		}
	}
}

- (void)performPlaybackDidPauseActions {
}

- (void)performPlaybackDidResumeActions {
}

- (void)performPlaybackDidStopActions {
	entry = nil;
}

- (void)awakeFromNib {
	[[NSNotificationCenter defaultCenter] addObserver:self
	                                         selector:@selector(playbackDidBegin:)
	                                             name:CogPlaybackDidBeginNotificiation
	                                           object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self
	                                         selector:@selector(playbackDidPause:)
	                                             name:CogPlaybackDidPauseNotificiation
	                                           object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self
	                                         selector:@selector(playbackDidResume:)
	                                             name:CogPlaybackDidResumeNotificiation
	                                           object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self
	                                         selector:@selector(playbackDidStop:)
	                                             name:CogPlaybackDidStopNotificiation
	                                           object:nil];
}

- (void)playbackDidBegin:(NSNotification *)notification {
	NSOperation *op = [NSBlockOperation blockOperationWithBlock:^{
		[self performPlaybackDidBeginActions:(PlaylistEntry *)[notification object]];
	}];
	[queue addOperation:op];
}

- (void)playbackDidPause:(NSNotification *)notification {
	NSOperation *op = [NSBlockOperation blockOperationWithBlock:^{
		[self performPlaybackDidPauseActions];
	}];
	[queue addOperation:op];
}

- (void)playbackDidResume:(NSNotification *)notification {
	NSOperation *op = [NSBlockOperation blockOperationWithBlock:^{
		[self performPlaybackDidResumeActions];
	}];
	[queue addOperation:op];
}

- (void)playbackDidStop:(NSNotification *)notification {
	NSOperation *op = [NSBlockOperation blockOperationWithBlock:^{
		[self performPlaybackDidStopActions];
	}];
	[queue addOperation:op];
}

@end
