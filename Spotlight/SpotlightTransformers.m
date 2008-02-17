//
//  SpotlightTransformers.m
//  Cog
//
//  Created by Matthew Grinshpun on 11/02/08.
//  Copyright 2008 Matthew Leon Grinshpun. All rights reserved.
//

#import "SpotlightTransformers.h"
#import "SpotlightWindowController.h"

// This is what we use instead of an outlet for PausingQueryTransformer
static SpotlightWindowController * searchController;

@implementation PausingQueryTransformer
+ (Class)transformedValueClass { return [NSArray class]; }
+ (BOOL)allowsReverseTransformation { return NO; }

+ (void)setSearchController:(SpotlightWindowController *)aSearchController
{
    searchController = aSearchController;
}

// Convert from string to NSURL
- (id)transformedValue:(id)value {
    // Rather unintuitively, this piece of code eliminates the "flicker"
    // when searching for new results, which resulted from a pause when the
    // search query stops gathering and sends an empty results array through KVO.
    if(([value count] > 0) || ([searchController.query isGathering]))
    {
        self.oldResults = (NSArray *)value;
    }
    return self.oldResults;
}

- (void)dealloc
{
    self.oldResults = nil;
    [super dealloc];
}

@synthesize oldResults;

@end