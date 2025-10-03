//
//  SpotlightWindowController.m
//  Cog
//
//  Created by Matthew Grinshpun on 10/02/08.
//  Copyright 2008 Matthew Leon Grinshpun. All rights reserved.
//

#import "SpotlightWindowController.h"
#import "NSArray+CogSort.h"
#import "NSComparisonPredicate+CogPredicate.h"
#import "NSNumber+CogSort.h"
#import "NSString+CogSort.h"
#import "PlaylistLoader.h"
#import "SpotlightPlaylistEntry.h"
#import "SpotlightTransformers.h"

#import "Logging.h"

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
	// Set the home directory as the default search directory
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	NSString *homeDir = @"~";
	homeDir = [homeDir stringByExpandingTildeInPath];
	homeDir = [[NSURL fileURLWithPath:homeDir isDirectory:YES] absoluteString];
	NSDictionary *searchDefault = @{@"spotlightSearchPath": homeDir};
	[defaults registerDefaults:searchDefault];
}

- (id)init {
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
		if(![self.query.predicate isEqual:spotlightPredicate] || ![self.query.searchScopes isEqualToArray:
		                                                                                   @[pathControl.URL]]) {
			if([self.query isStarted])
				[self.query stopQuery];
			self.query.predicate = spotlightPredicate;
			// Set scope to contents of pathControl
			self.query.searchScopes = @[pathControl.URL];
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
		                             @"sort": @(NO),
		                             @"origin": @(URLOriginExternal) };

	[playlistLoader performSelectorInBackground:@selector(addURLsInBackground:) withObject:loadEntryData];
	[self.query enableUpdates];
}

// If pop-up styled NSPathControl is set to /a/b/c path, then selecting either 'a' or 'b'
// from its pop-up menu won't do anything by default (while we'd like it to select /a and
// /a/b respectively). So here we set url of NSPathControl to be that of clicked cell.
- (IBAction)pathComponentClicked:(id)sender {
	NSPathComponentCell *pcc = [sender clickedPathComponentCell];
	DLog(@"%@", pcc);
	[sender setURL:[pcc URL]];
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
