//
//  SpotlightWindowController.m
//  Cog
//
//  Created by Matthew Grinshpun on 10/02/08.
//  Copyright 2008 Matthew Leon Grinshpun. All rights reserved.
//

#import "SpotlightWindowController.h"
#import "NSString+CogSort.h"
#import "PlaylistLoader.h"

#import "Logging.h"

#import "Cog-Swift.h"

#import <pwd.h>

// Minimum length of a search string (searching for very small strings gets ugly)
#define MINIMUM_SEARCH_STRING_LENGTH 3

// Store a class predicate for searching for music
static NSPredicate *musicOnlyPredicate = nil;

@implementation SpotlightWindowController

+ (void)initialize {
	musicOnlyPredicate = [NSPredicate predicateWithFormat:
	                                  @"kMDItemContentTypeTree==\'public.audio\'"];

	// Register value transformers
	NSValueTransformer *pausingQueryTransformer = [PausingQueryTransformer new];
	[NSValueTransformer setValueTransformer:pausingQueryTransformer forName:@"PausingQueryTransformer"];

	NSValueTransformer *authorToArtistTransformer = [AuthorToArtistTransformer new];
	[NSValueTransformer setValueTransformer:authorToArtistTransformer forName:@"AuthorToArtistTransformer"];

	NSValueTransformer *pathToURLTransformer = [PathToURLTransformer new];
	[NSValueTransformer setValueTransformer:pathToURLTransformer forName:@"PathToURLTransformer"];

	NSValueTransformer *stringToSearchScopeTransformer = [StringToSearchScopeTransformer new];
	[NSValueTransformer setValueTransformer:stringToSearchScopeTransformer forName:@"StringToSearchScopeTransformer"];
}

- (void)registerDefaults {
	// The home folder by default. NSHomeDirectory() is the sandbox container's,
	// where there is no music to find.
	NSString *homeDir = @"~";
	struct passwd *pw = getpwuid(getuid());
	if(pw && pw->pw_dir) homeDir = [NSString stringWithUTF8String:pw->pw_dir];
	else homeDir = [homeDir stringByExpandingTildeInPath];
	homeDir = [[NSURL fileURLWithPath:homeDir isDirectory:YES] absoluteString];
	[[NSUserDefaults standardUserDefaults] registerDefaults:@{@"spotlightSearchPath": homeDir}];
}

- (id)init {
	// With a nib name, NSWindowController calls loadWindow for the window;
	// that loads the nib, which has only the results table, and makes the
	// window around it.
	if(self = [super initWithWindowNibName:@"SpotlightPanel"]) {
		self.query = [NSMetadataQuery new];
		[self.query setDelegate:self];
		self.query.sortDescriptors = @[
			[[NSSortDescriptor alloc] initWithKey:@"kMDItemAuthors"
			                            ascending:YES
			                             selector:@selector(compareFirstString:)],
			[[NSSortDescriptor alloc] initWithKey:@"kMDItemAlbum"
			                            ascending:YES
			                             selector:@selector(caseInsensitiveCompare:)],
			[[NSSortDescriptor alloc] initWithKey:@"kMDItemAudioTrackNumber"
			                            ascending:YES
			                             selector:@selector(compareTrackNumbers:)]
		];

		// hook my query transformer up to me
		[PausingQueryTransformer setSearchController:self];

		[self registerDefaults];

		// TODO: spotlightSearchPath is bound via IB, is the below needed?
		//		NSDictionary *bindOptions =
		//			[NSDictionary dictionaryWithObject:@"StringToSearchScopeTransformer"
		//										forKey:NSValueTransformerNameBindingOption];
		//
		//		[self.query     bind:@"searchScopes"
		//					toObject:[NSUserDefaultsController sharedUserDefaultsController]
		//				 withKeyPath:@"values.spotlightSearchPath"
		//					 options:bindOptions];
	}

	return self;
}

- (void)awakeFromNib {
}

// The window is SwiftUI's; the nib holds only the results table, which it
// hosts. NSWindowController's own would look for the window in the nib.
- (void)loadWindow {
	[[NSBundle mainBundle] loadNibNamed:@"SpotlightPanel" owner:self topLevelObjects:nil];
	self.window = [SpotlightPanel makeWindowWithController:self];
}

- (NSArrayController *)resultsController {
	[self window];
	return playlistController;
}

- (NSURL *)searchFolder {
	NSString *string = [[NSUserDefaults standardUserDefaults] stringForKey:@"spotlightSearchPath"];
	NSURL *url = string ? [NSURL URLWithString:string] : nil;
	return url ?: [NSURL fileURLWithPath:NSHomeDirectory() isDirectory:YES];
}

- (void)setSearchFolder:(NSURL *)searchFolder {
	[[NSUserDefaults standardUserDefaults] setObject:[searchFolder absoluteString] forKey:@"spotlightSearchPath"];
	[self performSearch];
}

- (IBAction)toggleWindow:(id)sender {
	if([[self window] isVisible])
		[[self window] orderOut:self];
	else
		[self showWindow:self];
}

- (void)performSearch {
	NSPredicate *searchPredicate;
	// Process the search string into a compound predicate. If Nil is returned do nothing
	if((searchPredicate = [self processSearchString])) {
		// spotlightPredicate, which is what will finally be used for the spotlight search
		// is the union of the (potentially) compound searchPredicate and the static
		// musicOnlyPredicate

		NSPredicate *spotlightPredicate = [NSCompoundPredicate andPredicateWithSubpredicates:
		                                                       @[musicOnlyPredicate,
			                                                     searchPredicate]];
		// Only preform a new search if the predicate has changed or there is a new path
		NSURL *folder = self.searchFolder;
		if(![self.query.predicate isEqual:spotlightPredicate] || ![self.query.searchScopes isEqualToArray:@[folder]]) {
			if([self.query isStarted])
				[self.query stopQuery];
			self.query.predicate = spotlightPredicate;
			self.query.searchScopes = @[folder];
			[self.query startQuery];
			DLog(@"Started query: %@", [self.query.predicate description]);
		}
	}
}

- (NSPredicate *)processSearchString {
	NSMutableArray *subpredicates = [NSMutableArray arrayWithCapacity:10];

	NSScanner *scanner = [NSScanner scannerWithString:self.searchString];
	BOOL exactString;
	NSString *scannedString;
	NSMutableString *parsingString;
	while(![scanner isAtEnd]) {
		exactString = NO;
		if([scanner scanUpToString:@" " intoString:&scannedString]) {
			if([scannedString length] < MINIMUM_SEARCH_STRING_LENGTH)
				continue;

			// We use NSMutableString because this string will get abused a bit
			// It potentially could be reading the entire search string

			parsingString = [NSMutableString stringWithCapacity:[self.searchString length]];
			[parsingString setString:scannedString];

			if([parsingString characterAtIndex:0] == '%') {
				if([parsingString length] < (MINIMUM_SEARCH_STRING_LENGTH + 2))
					continue;

				if([parsingString characterAtIndex:2] == '\"') {
					exactString = YES;
					// If the string does not end in a quotation mark and we're not at the end,
					// scan until we find one.
					// Allows strings within quotation marks to include spaces
					if([parsingString characterAtIndex:([parsingString length] - 1)] != '\"' &&
					   ![scanner isAtEnd]) {
						NSString *restOfString;
						[scanner scanUpToString:@"\"" intoString:&restOfString];
						[parsingString appendFormat:@" %@", restOfString];
					} else if([parsingString characterAtIndex:([parsingString length] - 1)] == '\"') {
						// pick off the quotation mark at the end
						[parsingString deleteCharactersInRange:
						               NSMakeRange([parsingString length] - 1, 1)];
					}
					// eliminate beginning quotation mark
					[parsingString deleteCharactersInRange:NSMakeRange(2, 1)];
				}

				// Search for artist
				if([parsingString characterAtIndex:1] == 'a') {
					[subpredicates addObject:
					               [NSComparisonPredicate predicateForMdKey:@"kMDItemAuthors"
					                                             withString:[parsingString substringFromIndex:2]
					                                            exactString:exactString]];
				}

				// Search for album
				if([parsingString characterAtIndex:1] == 'l') {
					[subpredicates addObject:
					               [NSComparisonPredicate predicateForMdKey:@"kMDItemAlbum"
					                                             withString:[parsingString substringFromIndex:2]
					                                            exactString:exactString]];
				}

				// Search for title
				if([parsingString characterAtIndex:1] == 't') {
					[subpredicates addObject:
					               [NSComparisonPredicate predicateForMdKey:@"kMDItemTitle"
					                                             withString:[parsingString substringFromIndex:2]
					                                            exactString:exactString]];
				}

				// Search for genre
				if([parsingString characterAtIndex:1] == 'g') {
					[subpredicates addObject:
					               [NSComparisonPredicate predicateForMdKey:@"kMDItemMusicalGenre"
					                                             withString:[parsingString substringFromIndex:2]
					                                            exactString:exactString]];
				}

				// Search for comment
				if([parsingString characterAtIndex:1] == 'c') {
					[subpredicates addObject:
					               [NSComparisonPredicate predicateForMdKey:@"kMDItemComment"
					                                             withString:[parsingString substringFromIndex:2]
					                                            exactString:exactString]];
				}
			} else {
				NSString *wildcardString = [NSString stringWithFormat:@"*%@*", parsingString];
				NSPredicate *pred = [NSPredicate predicateWithFormat:@"(kMDItemTitle LIKE[cd] %@) OR (kMDItemAlbum LIKE[cd] %@) OR (kMDItemAuthors LIKE[cd] %@)",
				                                                     wildcardString, wildcardString, wildcardString];
				[subpredicates addObject:pred];
			}
		}
	}

	if([subpredicates count] == 0)
		return nil;
	else if([subpredicates count] == 1)
		return [subpredicates objectAtIndex:0];

	// Create a compound predicate from subPredicates
	return [NSCompoundPredicate andPredicateWithSubpredicates:subpredicates];
}

- (void)searchForArtist:(NSString *)artist {
	[self showWindow:self];
	self.searchString = [NSString stringWithFormat:@"%%a\"%@\"", artist];
}

- (void)searchForAlbum:(NSString *)album {
	[self showWindow:self];
	self.searchString = [NSString stringWithFormat:@"%%l\"%@\"", album];
}

- (void)dealloc {
	self.query = nil;
	self.searchString = nil;
}

- (IBAction)addToPlaylist:(id)sender {
	NSArray *tracks;
	[self.query disableUpdates];
	tracks = playlistController.selectedObjects;
	if([tracks count] == 0)
		tracks = playlistController.arrangedObjects;

	NSDictionary *loadEntryData = @{ @"entries": [tracks valueForKey:@"url"],
		                             @"sort": @NO,
		                             @"origin": @(URLOriginExternal) };

	[playlistLoader addURLsInBackground:loadEntryData];
	[self.query enableUpdates];
}

#pragma mark NSMetadataQuery delegate methods

// replace the NSMetadataItem with a PlaylistEntry
- (id)metadataQuery:(NSMetadataQuery *)query
replacementObjectForResultObject:(NSMetadataItem *)result {
	return [SpotlightPlaylistEntry playlistEntryWithMetadataItem:result];
}

#pragma mark Getters and setters

@synthesize query;

@synthesize searchString;
- (void)setSearchString:(NSString *)aString {
	// Make sure the string is changed
	if(![searchString isEqualToString:aString]) {
		searchString = [aString copy];
		[self performSearch];
	}
}

@end
