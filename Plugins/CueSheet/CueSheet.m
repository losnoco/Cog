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

@implementation CueSheet

+ (id)cueSheetWithFile:(NSString *)filename
{
	return [[[CueSheet alloc] initWithFile:filename] autorelease];
}

- (NSURL *)urlForPath:(NSString *)path relativeTo:(NSString *)baseFilename
{
	NSRange protocolRange = [path rangeOfString:@"://"];
	if (protocolRange.location != NSNotFound) 
	{
		return [NSURL URLWithString:path];
	}

	NSMutableString *unixPath = [path mutableCopy];

	//Get the fragment
	NSString *fragment = @"";
	NSScanner *scanner = [NSScanner scannerWithString:unixPath];
	NSCharacterSet *characterSet = [NSCharacterSet characterSetWithCharactersInString:@"#1234567890"];
	while (![scanner isAtEnd]) {
		NSString *possibleFragment;
		[scanner scanUpToString:@"#" intoString:nil];

		if ([scanner scanCharactersFromSet:characterSet intoString:&possibleFragment] && [scanner isAtEnd]) 
		{
			fragment = possibleFragment;
			[unixPath deleteCharactersInRange:NSMakeRange([scanner scanLocation] - [possibleFragment length], [possibleFragment length])];
			break;
		}
	}

	if (![unixPath hasPrefix:@"/"]) {
		//Only relative paths would have windows backslashes.
		[unixPath replaceOccurrencesOfString:@"\\" withString:@"/" options:0 range:NSMakeRange(0, [unixPath length])];
		
		NSString *basePath = [[[baseFilename stringByStandardizingPath] stringByDeletingLastPathComponent] stringByAppendingString:@"/"];

		[unixPath insertString:basePath atIndex:0];
	}
	
	//Append the fragment
	NSURL *url = [NSURL URLWithString:[[[NSURL fileURLWithPath:unixPath] absoluteString] stringByAppendingString: fragment]];
	[unixPath release];
	return url;
}




- (void)parseFile:(NSString *)filename
{
	NSStringEncoding encoding;
	NSError *error = nil;
	NSString *contents = [NSString stringWithContentsOfFile:filename usedEncoding:&encoding error:&error];
    if (error) {
        error = nil;
        contents = [NSString stringWithContentsOfFile:filename encoding:NSUTF8StringEncoding error:&error];
    }
    if (error) {
        error = nil;
        contents = [NSString stringWithContentsOfFile:filename encoding:NSWindowsCP1251StringEncoding error:&error];
	}
    if (error) {
        error = nil;
        contents = [NSString stringWithContentsOfFile:filename encoding:NSISOLatin1StringEncoding error:&error];
	}
	if (error || !contents) {
		NSLog(@"Could not open file...%@ %@ %@", filename, contents, error);
		return;
	}
	
	NSMutableArray *entries = [[NSMutableArray alloc] init];

	NSString *track = nil;
	NSString *path = nil;
	NSString *artist = nil;
	NSString *album = nil;
	NSString *title = nil;
	NSString *genre = nil;
	NSString *year = nil;
	
	BOOL trackAdded = NO;

	NSCharacterSet *whitespace = [NSCharacterSet whitespaceAndNewlineCharacterSet];

	NSString *line;
	NSScanner *scanner = nil;
	NSEnumerator *e = [[contents componentsSeparatedByString:@"\n"] objectEnumerator];
	while (line = [e nextObject])
	{
		[scanner release];
		scanner = [[NSScanner alloc] initWithString:line];

		NSString *command;
		if (![scanner scanUpToCharactersFromSet:whitespace intoString:&command]) {
			continue;
		}
		
		//FILE "filename.shn" WAVE
		if ([command isEqualToString:@"FILE"]) {
			trackAdded = NO;

			if (![scanner scanString:@"\"" intoString:nil]) {
				continue;
			}

			//Read in the path
			if (![scanner scanUpToString:@"\"" intoString:&path]) {
				continue;
			}
		}
		//TRACK 01 AUDIO
		else if ([command isEqualToString:@"TRACK"]) {
			trackAdded = NO;

			if (![scanner scanUpToCharactersFromSet:whitespace intoString:&track]) {
				continue;
			}
			
			NSString *type = nil;
			if (![scanner scanUpToCharactersFromSet:whitespace intoString:&type]
					|| ![type isEqualToString:@"AUDIO"]) {
				continue;
			}
		}
		//INDEX 01 00:00:10
		//Note that time is written in Minutes:Seconds:Frames, where frames are 1/75 of a second
		else if ([command isEqualToString:@"INDEX"]) {
			if (trackAdded) {
				continue;
			}
			
			if (!path) {
				continue;
			}

			NSString *index = nil;
			if (![scanner scanUpToCharactersFromSet:whitespace intoString:&index] || [index intValue] != 1) {
				continue;
			}
			
			[scanner scanCharactersFromSet:whitespace intoString:nil];

			NSString *time = nil;
			if (![scanner scanUpToCharactersFromSet:whitespace intoString:&time]) {
				continue;
			}
			
			NSArray *msf = [time componentsSeparatedByString:@":"];
			if ([msf count] != 3) {
				continue;
			}

			double seconds = (60*[[msf objectAtIndex:0] intValue]) + [[msf objectAtIndex:1] intValue] + ([[msf objectAtIndex:2] floatValue]/75);

			if (track == nil) {
				track = @"01";
			}

			//Need to add basePath, and convert to URL
			[entries addObject:
								[CueSheetTrack trackWithURL:[self urlForPath:path relativeTo:filename]
															track: track
															time: seconds 
															artist:artist 
															album:album 
															title:title
															genre:genre
															year:year]];
			trackAdded = YES;
		}
		else if ([command isEqualToString:@"PERFORMER"])
		{
			if (![scanner scanString:@"\"" intoString:nil]) {
				continue;
			}

			//Read in the path
			if (![scanner scanUpToString:@"\"" intoString:&artist]) {
				continue;
			}
		}
		else if ([command isEqualToString:@"TITLE"])
		{
			NSString **titleDest;
			if (!path) //Have not come across a file yet.
				titleDest = &album;
			else
				titleDest = &title;
			
			if (![scanner scanString:@"\"" intoString:nil]) {
				continue;
			}

			//Read in the path
			if (![scanner scanUpToString:@"\"" intoString:titleDest]) {
				continue;
			}
		}
		else if ([command isEqualToString:@"REM"]) //Additional metadata sometimes stored in comments
		{
			NSString *type;
			if ( ![scanner scanUpToCharactersFromSet:whitespace intoString:&type]) {
				continue;
			}
			
			if ([type isEqualToString:@"GENRE"])
			{
				//NSLog(@"GENRE!");
				if ([scanner scanString:@"\"" intoString:nil]) {
					//NSLog(@"QUOTED");
					if (![scanner scanUpToString:@"\"" intoString:&genre]) {
						NSLog(@"FAILED TO SCAN");
						continue;
					}
				}
				else {
					//NSLog(@"UNQUOTED");
					if ( ![scanner scanUpToCharactersFromSet:whitespace intoString:&genre]) {
						continue;
					}
				}
			}
			else if ([type isEqualToString:@"DATE"])
			{
				//NSLog(@"DATE!");
				if ( ![scanner scanUpToCharactersFromSet:whitespace intoString:&year]) {
					continue;
				}
			}
		}
	}

	[scanner release];
	
	tracks = [entries copy];

	[entries release];
}

- (id)initWithFile:(NSString *)filename
{
	self = [super init];
	if (self) {
		[self parseFile:filename];
	}
	
	return self;
}

- (void)dealloc
{
	[tracks release];
	
	[super dealloc];
}

- (NSArray *)tracks
{
	return tracks;
}

- (CueSheetTrack *)track:(NSString *)fragment
{
	CueSheetTrack *t;
	NSEnumerator *e = [tracks objectEnumerator];
	while (t = [e nextObject]) {
		if ([[t track] isEqualToString:fragment]) {
			return t;
		}
	}
	
	return nil;
}

@end
