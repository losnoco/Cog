//
//  CueSheet.m
//  CueSheet
//
//  Created by Zaphod Beeblebrox on 10/8/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "CueSheet.h"
#import "CueSheetTrack.h"

#import "Plugin.h"

#import "Logging.h"

#import "SandboxBroker.h"

@implementation CueSheet

+ (id)cueSheetWithFile:(NSString *)filename {
	return [[CueSheet alloc] initWithFile:filename];
}

+ (id)cueSheetWithString:(NSString *)cuesheet withFilename:(NSString *)filename {
	return [[CueSheet alloc] initWithString:cuesheet withFilename:filename];
}

- (NSURL *)urlForPath:(NSString *)path relativeTo:(NSString *)baseFilename {
	NSRange protocolRange = [path rangeOfString:@"://"];
	if(protocolRange.location != NSNotFound) {
		return [NSURL URLWithString:path];
	}

	NSMutableString *unixPath = [path mutableCopy];

	// Get the fragment
	NSString *fragment = @"";
	NSScanner *scanner = [NSScanner scannerWithString:unixPath];
	NSCharacterSet *characterSet = [NSCharacterSet characterSetWithCharactersInString:@"#1234567890"];
	while(![scanner isAtEnd]) {
		NSString *possibleFragment;
		[scanner scanUpToString:@"#" intoString:nil];

		if([scanner scanCharactersFromSet:characterSet intoString:&possibleFragment] && [scanner isAtEnd]) {
			fragment = possibleFragment;
			[unixPath deleteCharactersInRange:NSMakeRange([scanner scanLocation] - [possibleFragment length], [possibleFragment length])];
			break;
		}
	}

	if(![unixPath hasPrefix:@"/"]) {
		// Only relative paths would have windows backslashes.
		[unixPath replaceOccurrencesOfString:@"\\" withString:@"/" options:0 range:NSMakeRange(0, [unixPath length])];

		NSString *basePath = [[[baseFilename stringByStandardizingPath] stringByDeletingLastPathComponent] stringByAppendingString:@"/"];

		[unixPath insertString:basePath atIndex:0];
	}

	// Append the fragment
	NSURL *url = [NSURL URLWithString:[[[NSURL fileURLWithPath:unixPath] absoluteString] stringByAppendingString:fragment]];
	return url;
}

- (void)parseFile:(NSString *)filename {
	NSError *error = nil;
	id sandboxBrokerClass = NSClassFromString(@"SandboxBroker");
	const void *sbHandle = [[sandboxBrokerClass sharedSandboxBroker] beginFolderAccess:[NSURL fileURLWithPath:filename]];
	NSFileHandle *file = [NSFileHandle fileHandleForReadingAtPath:filename];
	NSData *data = nil;
	NSString *contents = nil;
	if(file) {
		if(@available(macOS 10.15, *)) {
			data = [file readDataToEndOfFileAndReturnError:&error];
		} else {
			data = [file readDataToEndOfFile];
			if(!data)
				error = [NSError errorWithDomain:NSPOSIXErrorDomain code:EIO userInfo:nil];
		}
	} else {
		error = [NSError errorWithDomain:NSPOSIXErrorDomain code:ENOENT userInfo:nil];
	}
	file = nil;
	[[sandboxBrokerClass sharedSandboxBroker] endFolderAccess:sbHandle];
	if(data) {
		NSMutableData *terminatedData = [data mutableCopy];
		const char z = '\0';
		[terminatedData appendBytes:&z length:sizeof(z)];
		contents = guess_encoding_of_string([terminatedData bytes]);
	}

	if(error || !contents) {
		ALog(@"Could not open file...%@ %@ %@", filename, contents, error);
		return;
	}

	[self parseString:contents withFilename:filename];
}

- (void)parseString:(NSString *)contents withFilename:(NSString *)filename {
	NSMutableArray *entries = [NSMutableArray new];

	NSString *track = nil;
	NSString *path = nil;
	NSString *artist = nil;
	NSString *album = nil;
	NSString *title = nil;
	NSString *genre = nil;
	NSString *year = nil;

	float albumGain = 0.0f;
	float albumPeak = 0.0f;
	float trackGain = 0.0f;
	float trackPeak = 0.0f;

	BOOL trackAdded = NO;

	NSCharacterSet *whitespace = [NSCharacterSet whitespaceAndNewlineCharacterSet];

	NSScanner *scanner = nil;
	for(NSString *line in [contents componentsSeparatedByString:@"\n"]) {
		scanner = [[NSScanner alloc] initWithString:line];

		NSString *command;
		if(![scanner scanUpToCharactersFromSet:whitespace intoString:&command]) {
			continue;
		}

		// FILE "filename.shn" WAVE
		if([command isEqualToString:@"FILE"]) {
			trackAdded = NO;

			if(![scanner scanString:@"\"" intoString:nil]) {
				continue;
			}

			// Read in the path
			if(![scanner scanUpToString:@"\"" intoString:&path]) {
				continue;
			}
		}
		// TRACK 01 AUDIO
		else if([command isEqualToString:@"TRACK"]) {
			trackAdded = NO;

			if(![scanner scanUpToCharactersFromSet:whitespace intoString:&track]) {
				continue;
			}

			NSString *type = nil;
			if(![scanner scanUpToCharactersFromSet:whitespace intoString:&type] || ![type isEqualToString:@"AUDIO"]) {
				continue;
			}
		}
		// INDEX 01 00:00:10
		// Note that time is written in Minutes:Seconds:Frames, where frames are 1/75 of a second
		else if([command isEqualToString:@"INDEX"]) {
			if(trackAdded) {
				continue;
			}

			if(!path) {
				continue;
			}

			NSString *index = nil;
			if(![scanner scanUpToCharactersFromSet:whitespace intoString:&index] || [index intValue] != 1) {
				continue;
			}

			[scanner scanCharactersFromSet:whitespace intoString:nil];

			NSString *time = nil;
			if(![scanner scanUpToCharactersFromSet:whitespace intoString:&time]) {
				continue;
			}

			NSArray *msf = [time componentsSeparatedByString:@":"];
			size_t count = [msf count];
			if(count != 1 && count != 3) {
				continue;
			}

			double seconds;
			BOOL timeInSamples = NO;

			if(count == 1) {
				seconds = [[msf objectAtIndex:0] floatValue];
				timeInSamples = YES;
			} else
				seconds = (60 * [[msf objectAtIndex:0] intValue]) + [[msf objectAtIndex:1] intValue] + ([[msf objectAtIndex:2] floatValue] / 75);

			if(track == nil) {
				track = @"01";
			}

			// Need to add basePath, and convert to URL
			[entries addObject:
			         [CueSheetTrack trackWithURL:[self urlForPath:path relativeTo:filename]
			                               track:track
			                                time:seconds
			                       timeInSamples:timeInSamples
			                              artist:artist
			                               album:album
			                               title:title
			                               genre:genre
			                                year:year
			                           albumGain:albumGain
			                           albumPeak:albumPeak
			                           trackGain:trackGain
			                           trackPeak:trackPeak]];
			trackAdded = YES;
		} else if([command isEqualToString:@"PERFORMER"]) {
			if(![scanner scanString:@"\"" intoString:nil]) {
				continue;
			}

			// Read in the path
			if(![scanner scanUpToString:@"\"" intoString:&artist]) {
				continue;
			}
		} else if([command isEqualToString:@"TITLE"]) {
			NSString *titleDest;

			if(![scanner scanString:@"\"" intoString:nil]) {
				continue;
			}

			// Read in the path
			if(![scanner scanUpToString:@"\"" intoString:&titleDest]) {
				continue;
			}

			if(!path) // Have not come across a file yet.
				album = titleDest;
			else
				title = titleDest;
		} else if([command isEqualToString:@"REM"]) // Additional metadata sometimes stored in comments
		{
			NSString *type;
			if(![scanner scanUpToCharactersFromSet:whitespace intoString:&type]) {
				continue;
			}

			if([type isEqualToString:@"GENRE"]) {
				// DLog(@"GENRE!");
				if([scanner scanString:@"\"" intoString:nil]) {
					// DLog(@"QUOTED");
					if(![scanner scanUpToString:@"\"" intoString:&genre]) {
						DLog(@"FAILED TO SCAN");
						continue;
					}
				} else {
					// DLog(@"UNQUOTED");
					if(![scanner scanUpToCharactersFromSet:whitespace intoString:&genre]) {
						continue;
					}
				}
			} else if([type isEqualToString:@"DATE"]) {
				// DLog(@"DATE!");
				if(![scanner scanUpToCharactersFromSet:whitespace intoString:&year]) {
					continue;
				}
			} else if([type hasPrefix:@"REPLAYGAIN_"]) {
				NSString *rgTag = nil;
				if(![scanner scanUpToCharactersFromSet:whitespace intoString:&rgTag])
					continue;
				if([type hasPrefix:@"REPLAYGAIN_ALBUM_"]) {
					if([type hasSuffix:@"GAIN"])
						albumGain = [rgTag floatValue];
					else if([type hasSuffix:@"PEAK"])
						albumPeak = [rgTag floatValue];
				} else if([type hasPrefix:@"REPLAYGAIN_TRACK_"]) {
					if([type hasSuffix:@"GAIN"])
						trackGain = [rgTag floatValue];
					else if([type hasSuffix:@"PEAK"])
						trackPeak = [rgTag floatValue];
				}
			}
		}
	}

	tracks = [entries copy];
}

- (id)initWithFile:(NSString *)filename {
	self = [super init];
	if(self) {
		[self parseFile:filename];
	}

	return self;
}

- (id)initWithString:(NSString *)cuesheet withFilename:(NSString *)filename {
	self = [super init];
	if(self) {
		[self parseString:cuesheet withFilename:filename];
	}

	return self;
}

- (NSArray *)tracks {
	return tracks;
}

- (CueSheetTrack *)track:(NSString *)fragment {
	for(CueSheetTrack *t in tracks) {
		if([[t track] isEqualToString:fragment]) {
			return t;
		}
	}

	return nil;
}

@end
