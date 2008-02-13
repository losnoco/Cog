//
//  SpotlightWindowController.m
//  Cog
//
//  Created by Matthew Grinshpun on 10/02/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "SpotlightWindowController.h"
#import "PlaylistLoader.h"
#import "SpotlightPlaylistEntry.h"

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
	if (self = [super init]) {
		self.query = [[NSMetadataQuery alloc] init];
        [self.query setDelegate:self];
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
    NSMutableArray *searchComponents = [NSMutableArray arrayWithCapacity:10];
    
    NSScanner *scanner = [NSScanner scannerWithString:self.searchString];
    
    while (![scanner isAtEnd])
    {
        NSString *scannedString;
        if ([scanner scanUpToString:@" " intoString:&scannedString])
        {
            // don't add tiny strings in... They're make the system go nuts
            // TODO: Maybe consider a better solution to the issue?
            if ([scannedString length] >= MINIMUM_SEARCH_STRING_LENGTH)
            {
                [searchComponents addObject:scannedString];
            }
        }
    }
    
    // create an array of all the predicates to join together
    NSMutableArray * subpredicates = [NSMutableArray 
        arrayWithCapacity:[searchComponents count]];
    
    // we will ignore case and diacritics
    unsigned options = (NSCaseInsensitivePredicateOption|
						NSDiacriticInsensitivePredicateOption);
    
    for(NSString *s in searchComponents)
    {
        // convert each "word" into "*word*"
        NSString *processedKey = [NSString stringWithFormat: @"*%@*", s];
        
        // Search all tags for something like word
        NSPredicate *predicate = [NSComparisonPredicate
                predicateWithLeftExpression:[NSExpression expressionForKeyPath:@"*"]
                            rightExpression:[NSExpression expressionForConstantValue:processedKey]
                                   modifier:NSDirectPredicateModifier
                                       type:NSLikePredicateOperatorType
                                    options:options];
        
        //TODO: Ability to search only artist, albums, etc.
        [subpredicates addObject: predicate];
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
	[super dealloc];
}

- (IBAction)addToPlaylist:(id)sender
{
    [self.query disableUpdates];
    
    NSArray *urls = [[playlistController selectedObjects]valueForKey:@"url"];
    [playlistLoader addURLs:urls sort:NO];
   
   [self.query enableUpdates];
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
// Normally, our nspathcontrol would just bind to the user defaults
// However, this does not allow us to perform a new search when
// the path changes. This getter/setter combo wraps around the user
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

@end
