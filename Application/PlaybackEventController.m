//
//  PlaybackEventController.m
//  Cog
//
//  Created by Vincent Spader on 3/5/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.

#import "PlaybackEventController.h"

#import "PlaylistEntry.h"

#if 0
NSString *TrackNotification = @"com.apple.iTunes.playerInfo";

NSString *TrackArtist = @"Artist";
NSString *TrackAlbum = @"Album";
NSString *TrackTitle = @"Name";
NSString *TrackGenre = @"Genre";
NSString *TrackNumber = @"Track Number";
NSString *TrackLength = @"Total Time";
NSString *TrackPath = @"Location";
NSString *TrackState = @"Player State";

typedef NS_ENUM(NSInteger, TrackStatus) { TrackPlaying,
	                                      TrackPaused,
	                                      TrackStopped };
#endif

@implementation PlaybackEventController {
	NSOperationQueue *queue;

	PlaylistEntry *entry;

	Boolean didGainUN API_AVAILABLE(macosx(10.14));
}

- (void)initDefaults {
	NSDictionary *defaultsDictionary = @{
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

		if(@available(macOS 10.14, *)) {
			UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];
			[center
			requestAuthorizationWithOptions:UNAuthorizationOptionAlert
			              completionHandler:^(BOOL granted, NSError *_Nullable error) {
				              self->didGainUN = granted;

				              if(granted) {
					              UNNotificationAction *skipAction = [UNNotificationAction
					              actionWithIdentifier:@"skip"
					                             title:@"Skip"
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

		[[NSUserNotificationCenter defaultUserNotificationCenter] setDelegate:self];

		entry = nil;
	}

	return self;
}

- (void)userNotificationCenter:(UNUserNotificationCenter *)center
       willPresentNotification:(UNNotification *)notification
         withCompletionHandler:
         (void (^)(UNNotificationPresentationOptions options))completionHandler
API_AVAILABLE(macos(10.14)) {
	UNNotificationPresentationOptions presentationOptions = UNNotificationPresentationOptionAlert;

	completionHandler(presentationOptions);
}

- (void)userNotificationCenter:(UNUserNotificationCenter *)center
didReceiveNotificationResponse:(UNNotificationResponse *)response
         withCompletionHandler:(void (^)(void))completionHandler API_AVAILABLE(macos(10.14)) {
	if([[response actionIdentifier] isEqualToString:@"skip"]) {
		[playbackController next:self];
	}
}

#if 0
- (NSDictionary *)fillNotificationDictionary:(PlaylistEntry *)pe status:(TrackStatus)status {
	NSMutableDictionary *dict = [NSMutableDictionary dictionary];
	if(pe == nil || pe.deLeted || pe.url == nil) return dict;

	[dict setObject:[pe.url absoluteString] forKey:TrackPath];
	if(pe.title) [dict setObject:pe.title forKey:TrackTitle];
	if(pe.artist) [dict setObject:pe.artist forKey:TrackArtist];
	if(pe.album) [dict setObject:pe.album forKey:TrackAlbum];
	if(pe.genre) [dict setObject:pe.genre forKey:TrackGenre];
	if(pe.track)
		[dict setObject:pe.trackText forKey:TrackNumber];
	if(pe.length)
		[dict setObject:@((NSInteger)([pe.length doubleValue] * 1000.0))
		         forKey:TrackLength];

	NSString *state = nil;

	switch(status) {
		case TrackPlaying:
			state = @"Playing";
			break;
		case TrackPaused:
			state = @"Paused";
			break;
		case TrackStopped:
			state = @"Stopped";
			break;
		default:
			break;
	}

	[dict setObject:state forKey:TrackState];

	return dict;
}
#endif

- (void)performPlaybackDidBeginActions:(PlaylistEntry *)pe {
	if(NO == [pe error]) {
		entry = pe;

#if 0
		[[NSDistributedNotificationCenter defaultCenter]
		postNotificationName:TrackNotification
		              object:nil
		            userInfo:[self fillNotificationDictionary:pe status:TrackPlaying]
		  deliverImmediately:YES];
#endif

		NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

		if([defaults boolForKey:@"notifications.enable"]) {
			if(@available(macOS 10.14, *)) {
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
			} else {
				NSUserNotification *notif = [NSUserNotification new];
				notif.title = [pe title];

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

				if([defaults boolForKey:@"notifications.itunes-style"]) {
					notif.subtitle = subtitle;
					[notif setValue:@YES forKey:@"_showsButtons"];
				} else {
					notif.informativeText = subtitle;
				}

				if([notif respondsToSelector:@selector(setContentImage:)]) {
					if([defaults boolForKey:@"notifications.show-album-art"] &&
					   [pe albumArtInternal]) {
						NSImage *image = [pe albumArt];

						if([defaults boolForKey:@"notifications.itunes-style"]) {
							[notif setValue:image forKey:@"_identityImage"];
						} else {
							notif.contentImage = image;
						}
					}
				}

				notif.actionButtonTitle = NSLocalizedString(@"SkipAction", @"");

				[[NSUserNotificationCenter defaultUserNotificationCenter]
				scheduleNotification:notif];
			}
		}
	}
}

- (void)performPlaybackDidPauseActions {
#if 0
	[[NSDistributedNotificationCenter defaultCenter]
	postNotificationName:TrackNotification
	              object:nil
	            userInfo:[self fillNotificationDictionary:entry status:TrackPaused]
	  deliverImmediately:YES];
#endif
}

- (void)performPlaybackDidResumeActions {
#if 0
	[[NSDistributedNotificationCenter defaultCenter]
	postNotificationName:TrackNotification
	              object:nil
	            userInfo:[self fillNotificationDictionary:entry status:TrackPlaying]
	  deliverImmediately:YES];
#endif
}

- (void)performPlaybackDidStopActions {
#if 0
	[[NSDistributedNotificationCenter defaultCenter]
	postNotificationName:TrackNotification
	              object:nil
	            userInfo:[self fillNotificationDictionary:entry status:TrackStopped]
	  deliverImmediately:YES];
#endif
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

- (void)userNotificationCenter:(NSUserNotificationCenter *)center
       didActivateNotification:(NSUserNotification *)notification {
	switch(notification.activationType) {
		case NSUserNotificationActivationTypeActionButtonClicked:
			[playbackController next:self];
			break;

		case NSUserNotificationActivationTypeContentsClicked: {
			NSWindow *window = [[NSUserDefaults standardUserDefaults] boolForKey:@"miniMode"] ? miniWindow : mainWindow;

			[NSApp activateIgnoringOtherApps:YES];
			[window makeKeyAndOrderFront:self];
		}; break;

		default:
			break;
	}
}

@end
