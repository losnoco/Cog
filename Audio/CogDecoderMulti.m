//
//  CogDecoderMulti.m
//  CogAudio
//
//  Created by Christopher Snowhill on 10/21/13.
//
//

#import "CogDecoderMulti.h"

@implementation CogDecoderMulti

+ (NSArray *)mimeTypes
{
    return nil;
}

+ (NSArray *)fileTypes
{
    return nil;
}

+ (float)priority
{
    return -1.0;
}

- (id)initWithDecoders:(NSArray *)decoders
{
    self = [super init];
    if ( self )
    {
        NSMutableArray *sortedDecoders = [NSMutableArray arrayWithArray:decoders];
        [sortedDecoders sortUsingComparator:
         ^NSComparisonResult(id obj1, id obj2)
         {
             NSString *classString1 = (NSString *)obj1;
             NSString *classString2 = (NSString *)obj2;
             
             Class decoder1 = NSClassFromString(classString1);
             Class decoder2 = NSClassFromString(classString2);
             
             float priority1 = [decoder1 priority];
             float priority2 = [decoder2 priority];
             
             if (priority1 == priority2) return NSOrderedSame;
             else if (priority1 > priority2) return NSOrderedAscending;
             else return NSOrderedDescending;
         }];
        theDecoders = sortedDecoders;
        theDecoder = nil;
        cachedObservers = [[[NSMutableArray alloc] init] autorelease];
    }
    return self;
}

- (NSDictionary *)properties
{
    if ( theDecoder != nil ) return [theDecoder properties];
    return nil;
}

- (int)readAudio:(void *)buffer frames:(UInt32)frames
{
    if ( theDecoder != nil ) return [theDecoder readAudio:buffer frames:frames];
    return 0;
}

- (BOOL)open:(id<CogSource>)source
{
    for (NSString *classString in theDecoders)
    {
        Class decoder = NSClassFromString(classString);
        theDecoder = [[decoder alloc] init];
        for (NSDictionary *obsItem in cachedObservers) {
            [theDecoder addObserver:[obsItem objectForKey:@"observer"] forKeyPath:[obsItem objectForKey:@"keyPath"] options:[obsItem objectForKey:@"options"] context:[obsItem objectForKey:@"context"]];
        }
        if ([theDecoder open:source])
            return YES;
        for (NSDictionary *obsItem in cachedObservers) {
            [theDecoder removeObserver:[obsItem objectForKey:@"observer"] forKeyPath:[obsItem objectForKey:@"keyPath"]];
        }
        [theDecoder release];
        [source seek:0 whence:SEEK_SET];
    }
    theDecoder = nil;
    return NO;
}

- (long)seek:(long)frame
{
    if ( theDecoder != nil ) return [theDecoder seek:frame];
    return -1;
}

- (void)close
{
    if ( theDecoder != nil ) {
        [theDecoder close];
        [theDecoder release];
        theDecoder = nil;
    }
}

- (BOOL)setTrack:(NSURL *)track
{
    if ( theDecoder != nil && [theDecoder respondsToSelector: @selector(setTrack:)] ) return [theDecoder setTrack:track];
    return NO;
}

/* By the current design, the core adds its observers to decoders before they are opened */
- (void)addObserver:(NSObject *)observer forKeyPath:(NSString *)keyPath options:(NSKeyValueObservingOptions)options context:(void *)context
{
    [cachedObservers addObject:[NSDictionary dictionaryWithObjectsAndKeys:observer, @"observer", keyPath, @"keyPath", options, @"options", context, @"context", nil]];
}

- (void)removeObserver:(NSObject *)observer forKeyPath:(NSString *)keyPath
{
    if ( theDecoder != nil )
        [theDecoder removeObserver:observer forKeyPath:keyPath];
    for (NSDictionary *obsItem in cachedObservers)
    {
        if ([obsItem objectForKey:@"observer"] == observer && [keyPath isEqualToString:[obsItem objectForKey:@"keyPath"]]) {
            [cachedObservers removeObject:obsItem];
        }
    }
}

@end
