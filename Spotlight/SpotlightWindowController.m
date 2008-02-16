//
//  SpotlightWindowController.m
//  Cog
//
//  Created by Matthew Grinshpun on 10/02/08.
//  Copyright 2008 Matthew Leon Grinshpun. All rights reserved.
//

#import "SpotlightWindowController.h"
#import "PlaylistLoader.h"
#import "SpotlightPlaylistEntry.h"
#import "NSComparisonPredicate+CogPredicate.h"
#import "NSArray+CogSort.h"
#import "NSString+CogSort.h"
#import "NSNumber+CogSort.h"

// Minimum length of a search string (searching for very small strings gets ugly)
#define MINIMUM_SEARCH_STRING_LENGTH 3

// Store a class predicate for searching for music
static NSPredicate * musicOnlyPredicate = nil;

@implementation SpotlightWindowController

+ (void)initialize
{
	musicOnlyPredicate = [[NSPredicate predicateWithFormat:
                        @"kMDItemContentTypeTree==\'public.audio\'"] retain];

    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    
    // Set the home directory as the default search directory
    NSString * homeDir = @"~";
    homeDir = [homeDir stringByExpandingTildeInPath];
    homeDir = [[NSURL fileURLWithPath:homeDir isDirectory:YES] absoluteString];
    NSDictionary *searchDefault = 
                        [NSDictionary dictionaryWithObject:homeDir
                                                    forKey:@"spotlightSearchPath"];
    [defaults registerDefaults:searchDefault];
}

- (id)init
{
	if (self = [super initWithWindowNibName:@"SpotlightPanel"]) {
		self.query = [[NSMetadataQuery alloc] init];
        [self.query setDelegate:self];
        self.query.sortDescriptors = [NSArray arrayWithObjects:
        [[NSSortDescriptor alloc]initWithKey:@"kMDItemAuthors"
                                   ascending:YES
                                    selector:@selector(compareFirstString:)],
        [[NSSortDescriptor alloc]initWithKey:@"kMDItemAlbum"
                                   ascending:YES
                                    selector:@selector(caseInsensitiveCompare:)],
        [[NSSortDescriptor alloc]initWithKey:@"kMDItemAudioTrackNumber"
                                   ascending:YES
                                    selector:@selector(compareTrackNumbers:)],
        Nil];
	}

    return self;
}

- (void)performSearch
{
    NSPredicate *searchPredicate;
    // Process the search string into a compound predicate. If Nil is returned do nothing
    if(searchPredicate = [self processSearchString])
    {
        // spotlightPredicate, which is what will finally be used for the spotlight search
        // is the union of the (potentially) compound searchPredicate and the static 
        // musicOnlyPredicate
    
        NSPredicate *spotlightPredicate = [NSCompoundPredicate andPredicateWithSubpredicates:
                                           [NSArray arrayWithObjects: musicOnlyPredicate,
                                                                searchPredicate,
                                                                nil]];
        // Only preform a new search if the predicate has changed or there is a new path
        if(![self.query.predicate isEqual:spotlightPredicate]
            || ![self.query.searchScopes isEqualToArray:
                [NSArray arrayWithObjects:pathControl.URL, nil]])
        {
            if([self.query isStarted])
                [self.query stopQuery];
            self.query.predicate = spotlightPredicate;
            // Set scope to contents of pathControl
            self.query.searchScopes = [NSArray arrayWithObjects:pathControl.URL, nil];
            [self.query startQuery];
            NSLog(@"Started query: %@", [self.query.predicate description]);
        }
    }
}

- (NSPredicate *)processSearchString
{
    NSMutableArray *subpredicates = [NSMutableArray arrayWithCapacity:10];
    
    NSScanner *scanner = [NSScanner scannerWithString:self.searchString];
    BOOL exactString;
    NSString * scannedString;
    NSMutableString * parsingString;
    while (![scanner isAtEnd])
    {
        exactString = NO;
        if ([scanner scanUpToString:@" " intoString:&scannedString])
        {
            if ([scannedString length] < MINIMUM_SEARCH_STRING_LENGTH)
                continue;
                
            // We use NSMutableString because this string will get abused a bit
            // It potentially could be reading the entire search string
            
            parsingString = [NSMutableString stringWithCapacity: [self.searchString length]];
            [parsingString setString: scannedString];
            
                
            if ([parsingString characterAtIndex:0] == '%')
            {
                if ([parsingString length] < (MINIMUM_SEARCH_STRING_LENGTH + 2))
                    continue;
                
                if ([parsingString characterAtIndex:2] == '\"')
                {
                    exactString = YES;
                    // If the string does not end in a quotation mark and we're not at the end, 
                    // scan until we find one.
                    // Allows strings within quotation marks to include spaces
                    if ([parsingString characterAtIndex:([parsingString length] - 1)] != '\"' &&
                        ![scanner isAtEnd])
                    {
                        NSString *restOfString;
                        [scanner scanUpToString:@"\"" intoString:&restOfString];
                        [parsingString appendFormat:@" %@", restOfString];
                    }
                    else if ([parsingString characterAtIndex:([parsingString length] - 1)] == '\"')
                    {
                        // pick off the quotation mark at the end
                        [parsingString deleteCharactersInRange:
                            NSMakeRange([parsingString length] - 1, 1)];
                        
                    }
                    // eliminate beginning quotation mark
                    [parsingString deleteCharactersInRange: NSMakeRange(2, 1)];
                }
                    
                // Search for artist
                if([parsingString characterAtIndex:1] == 'a')
                {
                    [subpredicates addObject: 
                        [NSComparisonPredicate predicateForMdKey:@"kMDItemAuthors"
                                                      withString:[parsingString substringFromIndex:2]
                                                     exactString:exactString]];
                }
                
                // Search for album
                if([parsingString characterAtIndex:1] == 'l')
                {
                    [subpredicates addObject: 
                        [NSComparisonPredicate predicateForMdKey:@"kMDItemAlbum"
                                                      withString:[parsingString substringFromIndex:2]
                                                     exactString:exactString]];
                }
                
                // Search for title
                if([parsingString characterAtIndex:1] == 't')
                {
                    [subpredicates addObject: 
                        [NSComparisonPredicate predicateForMdKey:@"kMDItemTitle"
                                                      withString:[parsingString substringFromIndex:2]
                                                     exactString:exactString]];
                }
                
                // Search for genre
                if([parsingString characterAtIndex:1] == 'g')
                {
                    [subpredicates addObject: 
                        [NSComparisonPredicate predicateForMdKey:@"kMDItemMusicalGenre"
                                                      withString:[parsingString substringFromIndex:2]
                                                     exactString:exactString]];
                }
                
                // Search for comment
                if([parsingString characterAtIndex:1] == 'c')
                {
                    [subpredicates addObject: 
                        [NSComparisonPredicate predicateForMdKey:@"kMDItemComment"
                                                      withString:[parsingString substringFromIndex:2]
                                                     exactString:exactString]];
                }
            }
            else
            {
				NSString * wildcardString = [NSString stringWithFormat:@"*%@*", parsingString];
                NSPredicate * pred =[NSPredicate predicateWithFormat:@"(kMDItemTitle LIKE[cd] %@) OR (kMDItemAlbum LIKE[cd] %@) OR (kMDItemAuthors LIKE[cd] %@)",
                    wildcardString, wildcardString, wildcardString];
                [subpredicates addObject: pred];
            }
        }
    }
    
    if ([subpredicates count] == 0)
        return Nil;
    else if ([subpredicates count] == 1)
        return [subpredicates objectAtIndex: 0];
    
    // Create a compound predicate from subPredicates
    return [NSCompoundPredicate andPredicateWithSubpredicates: subpredicates];
}

- (void)dealloc
{
	[self.query stopQuery];
	[self.query release];
	[self.searchString release];
    [musicOnlyPredicate release];
    [self.oldResults release];
	[super dealloc];
}

- (IBAction)addToPlaylist:(id)sender
{
    NSArray *tracks;
    [self.query disableUpdates];
    tracks = playlistController.selectedObjects;
    if ([tracks count] == 0)
        tracks = playlistController.arrangedObjects;
    [playlistLoader addURLs:[tracks valueForKey:@"url"] sort:NO];
   
   [self.query enableUpdates];
}

// Don't update the track list until some results have been gathered
- (id)valueForKeyPath:(NSString *)keyPath
{
    if([keyPath isEqualToString:@"query.results"])
    {
        if(([self.query.results count] == 0) && [self.query isGathering])
            return self.oldResults;
        self.oldResults = [NSArray arrayWithArray:self.query.results];
        return self.oldResults;
    }
    return [super valueForKeyPath:keyPath];
}

#pragma mark NSMetadataQuery delegate methods

// replace the NSMetadataItem with a PlaylistEntry
- (id)metadataQuery:(NSMetadataQuery*)query
replacementObjectForResultObject:(NSMetadataItem*)result
{
    return [SpotlightPlaylistEntry playlistEntryWithMetadataItem: result];
}

#pragma mark Getters and setters

@synthesize query;

@synthesize searchString;
- (void)setSearchString:(NSString *)aString 
{
	// Make sure the string is changed
    if (![searchString isEqualToString:aString]) 
	{
		searchString = [aString copy];
        [self performSearch];
	}
}

@dynamic spotlightSearchPath;
// getter reads from user defaults
- (NSString *)spotlightSearchPath
{
    return [[[NSUserDefaults standardUserDefaults] 
        stringForKey:@"spotlightSearchPath"]copy];
}
// Normally, our NSPathcontrol would just bind to the user defaults
// However, this does not allow us to perform a new search when
// the path changes. This getter/setter combo wraps the user
// defaults while performing a new search when the value changes.
- (void)setSpotlightSearchPath:(NSString *)aString
{
    // Make sure the string is changed
	if (![spotlightSearchPath isEqualToString: aString]) 
	{
		spotlightSearchPath = [aString copy];
        [[NSUserDefaults standardUserDefaults] setObject:spotlightSearchPath
                                                  forKey:@"spotlightSearchPath"];
        [self performSearch];
	}
}

@synthesize oldResults;

@end
