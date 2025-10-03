//
//  RedundantPlaylistDataStore.m
//  Cog
//
//  Created by Christopher Snowhill on 2/16/22.
//

// Coalesce an entryInfo dictionary from tag loading into a common data dictionary, to
// reduce the memory footprint of adding a lot of tracks to the playlist.

#import "RedundantPlaylistDataStore.h"

#import "SHA256Digest.h"

@implementation RedundantPlaylistDataStore

- (id)init {
	self = [super init];

	if(self) {
		stringStore = [NSMutableArray new];
		artStore = [NSMutableDictionary new];
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

	NSString *key = [SHA256Digest digestDataAsString:in];

	NSData *ret = [artStore objectForKey:key];
	if(ret == nil) {
		[artStore setObject:in forKey:key];
		return in;
	} else {
		return ret;
	}
}

- (NSArray *)coalesceArray:(NSArray *)input {
	if(input == nil) return input;

	NSMutableArray *array = [NSMutableArray new];
	for(NSString *string in input) {
		[array addObject:[self coalesceString:string]];
	}

	return [NSArray arrayWithArray:array];
}

- (NSDictionary *)coalesceEntryInfo:(NSDictionary *)entryInfo {
	if(entryInfo == nil) return entryInfo;

	__block NSMutableDictionary *ret = [[NSMutableDictionary alloc] initWithCapacity:[entryInfo count]];

	[entryInfo enumerateKeysAndObjectsUsingBlock:^(id _Nonnull key, id _Nonnull obj, BOOL *_Nonnull stop) {
		if([obj isKindOfClass:[NSString class]]) {
			NSString *stringObj = (NSString *)obj;
			[ret setObject:[self coalesceString:stringObj] forKey:key];
		} else if([obj isKindOfClass:[NSData class]]) {
			NSData *dataObj = (NSData *)obj;
			[ret setObject:[self coalesceArt:dataObj] forKey:key];
		} else if([obj isKindOfClass:[NSArray class]]) {
			NSArray *arrayObj = (NSArray *)obj;
			[ret setObject:[self coalesceArray:arrayObj] forKey:key];
		} else {
			[ret setObject:obj forKey:key];
		}
	}];

	return [NSDictionary dictionaryWithDictionary:ret];
}

- (void)reset {
	[stringStore removeAllObjects];
	[artStore removeAllObjects];
}

@end
