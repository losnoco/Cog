//
//  RedundantPlaylistDataStore.m
//  Cog
//
//  Created by Christopher Snowhill on 2/16/22.
//

// Coalesce an entryInfo dictionary from tag loading into a common data dictionary, to
// reduce the memory footprint of adding a lot of tracks to the playlist.

#import "RedundantPlaylistDataStore.h"

@implementation RedundantPlaylistDataStore

- (id)init {
	self = [super init];

	if(self) {
		stringStore = [[NSMutableArray alloc] init];
		artStore = [[NSMutableArray alloc] init];
	}

	return self;
}

- (NSString *)coalesceString:(NSString *)in {
	if(in == nil) return in;

	NSUInteger index = [stringStore indexOfObject:in];
	if(index == NSNotFound) {
		[stringStore addObject:in];
		return in;
	} else {
		return [stringStore objectAtIndex:index];
	}
}

- (NSData *)coalesceArt:(NSData *)in {
	if(in == nil) return in;

	NSUInteger index = [artStore indexOfObject:in];
	if(index == NSNotFound) {
		[artStore addObject:in];
		return in;
	} else {
		return [artStore objectAtIndex:index];
	}
}

- (NSDictionary *)coalesceEntryInfo:(NSDictionary *)entryInfo {
	__block NSMutableDictionary *ret = [[NSMutableDictionary alloc] initWithCapacity:[entryInfo count]];

	[entryInfo enumerateKeysAndObjectsUsingBlock:^(id _Nonnull key, id _Nonnull obj, BOOL *_Nonnull stop) {
		if([obj isKindOfClass:[NSString class]]) {
			NSString *stringObj = (NSString *)obj;
			[ret setObject:[self coalesceString:stringObj] forKey:key];
		} else if([obj isKindOfClass:[NSData class]]) {
			NSData *dataObj = (NSData *)obj;
			[ret setObject:[self coalesceArt:dataObj] forKey:key];
		} else {
			[ret setObject:obj forKey:key];
		}
	}];

	return [NSDictionary dictionaryWithDictionary:ret];
}

@end
