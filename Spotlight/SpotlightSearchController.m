//
//  SpotlightSearchController.m
//  Cog
//
//  Created by Matthew Grinshpun on 10/02/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "SpotlightSearchController.h"
#import "SpotlightWindowController.h"
#import "PlaylistLoader.h"
#import "SpotlightPlaylistEntry.h"

// Store a class predicate for searching for music
static NSPredicate * musicOnlyPredicate = nil;

@implementation SpotlightSearchController

+ (void)initialize
{
	musicOnlyPredicate = [[NSPredicate predicateWithFormat:
                        @"kMDItemContentTypeTree==\'public.audio\'"] retain];
}

- (id)init
{
	if (self = [super init]) {
		self.query = [[NSMetadataQuery alloc] init];
        [self.query setDelegate:self];
	}

    return self;
}

- (void)awakeFromNib
{
    // Will replace this with userdefaults, but for now just start in
    // the home directory.
    NSString *homeDirectory = @"~";
    homeDirectory = [homeDirectory stringByExpandingTildeInPath];
    [pathControl setURL:[NSURL fileURLWithPath:homeDirectory isDirectory:YES]];
}

- (void)performSearch
{
    unsigned options = (NSCaseInsensitivePredicateOption|
                        NSDiacriticInsensitivePredicateOption);
    [self.query setSearchScopes:[NSArray arrayWithObjects:pathControl.URL, nil]];
	NSString *processedKey = [NSString stringWithFormat: @"*%@*", self.searchString];
	
	NSPredicate *searchPredicate = [NSComparisonPredicate
	         predicateWithLeftExpression:[NSExpression expressionForKeyPath:@"*"]
	                     rightExpression:[NSExpression expressionForConstantValue:processedKey]
	                            modifier:NSDirectPredicateModifier
	                                type:NSLikePredicateOperatorType
	                             options:options];
    
    // spotlightPredicate, which is what will finally be used for the spotlight search
    // is the union of the bound NSSearchField and the static musicOnlyPredicate
    
    NSPredicate *spotlightPredicate = [NSCompoundPredicate andPredicateWithSubpredicates:
                                       [NSArray arrayWithObjects: musicOnlyPredicate,
                                                            searchPredicate,
                                                            nil]];
    if([self.query isStarted])
        [self.query stopQuery];
    self.query.predicate = spotlightPredicate;
    [self.query startQuery];
    NSLog(@"Started query: %@", [self.query.predicate description], [[self.query class]description]);
}

- (void)dealloc
{
	[self.query stopQuery];
	[self.query release];
	[self.searchString release];
    [musicOnlyPredicate release];
	[super dealloc];
}

- (IBAction)changeSearchPath:(id)sender
{
    // When the search path is changed, restart search
    if([self.query isStarted]) {
        [self performSearch];
    }
}

- (IBAction)addToPlaylist:(id)sender
{
    [self.query disableUpdates];
    
    NSArray *songURLs = [[playlistController selectedObjects]valueForKey:@"url"];
    [spotlightWindowController.playlistLoader addURLs:songURLs sort:NO];
   
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
	if (searchString != aString) {
		searchString = [aString copy];
        [self performSearch];
	}
}

@end
