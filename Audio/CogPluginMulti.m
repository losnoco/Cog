//
//  CogPluginMulti.m
//  CogAudio
//
//  Created by Christopher Snowhill on 10/21/13.
//
//

#import "CogPluginMulti.h"

NSArray *sortClassesByPriority(NSArray *theClasses) {
	NSMutableArray *sortedClasses = [NSMutableArray arrayWithArray:theClasses];
	[sortedClasses sortUsingComparator:
	               ^NSComparisonResult(id obj1, id obj2) {
		               NSString *classString1 = (NSString *)obj1;
		               NSString *classString2 = (NSString *)obj2;

		               Class class1 = NSClassFromString(classString1);
		               Class class2 = NSClassFromString(classString2);

		               float priority1 = [class1 priority];
		               float priority2 = [class2 priority];

		               if(priority1 == priority2)
			               return NSOrderedSame;
		               else if(priority1 > priority2)
			               return NSOrderedAscending;
		               else
			               return NSOrderedDescending;
	               }];
	return sortedClasses;
}

@implementation CogDecoderMulti

+ (NSArray *)mimeTypes {
	return nil;
}

+ (NSArray *)fileTypes {
	return nil;
}

+ (float)priority {
	return -1.0;
}

+ (NSArray *)fileTypeAssociations {
	return nil;
}

- (id)initWithDecoders:(NSArray *)decoders {
	self = [super init];
	if(self) {
		theDecoders = sortClassesByPriority(decoders);
		theDecoder = nil;
		cachedObservers = [[NSMutableArray alloc] init];
	}
	return self;
}

- (NSDictionary *)properties {
	if(theDecoder != nil) return [theDecoder properties];
	return nil;
}

- (int)readAudio:(void *)buffer frames:(UInt32)frames {
	if(theDecoder != nil) return [theDecoder readAudio:buffer frames:frames];
	return 0;
}

- (BOOL)open:(id<CogSource>)source {
	for(NSString *classString in theDecoders) {
		Class decoder = NSClassFromString(classString);
		theDecoder = [[decoder alloc] init];
		for(NSDictionary *obsItem in cachedObservers) {
			[theDecoder addObserver:[obsItem objectForKey:@"observer"]
			             forKeyPath:[obsItem objectForKey:@"keyPath"]
			                options:[[obsItem objectForKey:@"options"] unsignedIntegerValue]
			                context:(__bridge void *)([obsItem objectForKey:@"context"])];
		}
		if([theDecoder open:source])
			return YES;
		for(NSDictionary *obsItem in cachedObservers) {
			[theDecoder removeObserver:[obsItem objectForKey:@"observer"] forKeyPath:[obsItem objectForKey:@"keyPath"]];
		}
		if([source seekable])
			[source seek:0 whence:SEEK_SET];
	}
	theDecoder = nil;
	return NO;
}

- (long)seek:(long)frame {
	if(theDecoder != nil) return [theDecoder seek:frame];
	return -1;
}

- (void)close {
	if(theDecoder != nil) {
		for(NSDictionary *obsItem in cachedObservers) {
			[theDecoder removeObserver:[obsItem objectForKey:@"observer"] forKeyPath:[obsItem objectForKey:@"keyPath"]];
		}
		[cachedObservers removeAllObjects];
		[theDecoder close];
		theDecoder = nil;
	}
}

- (BOOL)setTrack:(NSURL *)track {
	if(theDecoder != nil && [theDecoder respondsToSelector:@selector(setTrack:)]) return [theDecoder setTrack:track];
	return NO;
}

/* By the current design, the core adds its observers to decoders before they are opened */
- (void)addObserver:(NSObject *)observer forKeyPath:(NSString *)keyPath options:(NSKeyValueObservingOptions)options context:(void *)context {
	if(context != nil) {
		[cachedObservers addObject:[NSDictionary dictionaryWithObjectsAndKeys:observer, @"observer", keyPath, @"keyPath", @(options), @"options", context, @"context", nil]];
	} else {
		[cachedObservers addObject:[NSDictionary dictionaryWithObjectsAndKeys:observer, @"observer", keyPath, @"keyPath", @(options), @"options", nil]];
	}
	if(theDecoder) {
		[theDecoder addObserver:observer forKeyPath:keyPath options:options context:context];
	}
}

/* And this is currently called after the decoder is closed */
- (void)removeObserver:(NSObject *)observer forKeyPath:(NSString *)keyPath {
	for(NSDictionary *obsItem in cachedObservers) {
		if([obsItem objectForKey:@"observer"] == observer && [keyPath isEqualToString:[obsItem objectForKey:@"keyPath"]]) {
			[cachedObservers removeObject:obsItem];
			break;
		}
	}
	if(theDecoder) {
		[theDecoder removeObserver:observer forKeyPath:keyPath];
	}
}

@end

@implementation CogContainerMulti

+ (NSArray *)urlsForContainerURL:(NSURL *)url containers:(NSArray *)containers {
	NSArray *sortedContainers = sortClassesByPriority(containers);
	for(NSString *classString in sortedContainers) {
		Class container = NSClassFromString(classString);
		NSArray *urls = [container urlsForContainerURL:url];
		if([urls count])
			return urls;
	}
	return nil;
}

@end

@implementation CogMetadataReaderMulti

+ (NSDictionary *)metadataForURL:(NSURL *)url readers:(NSArray *)readers {
	NSArray *sortedReaders = sortClassesByPriority(readers);
	for(NSString *classString in sortedReaders) {
		Class reader = NSClassFromString(classString);
		NSDictionary *data = [reader metadataForURL:url];
		if([data count])
			return data;
	}
	return nil;
}

@end

@implementation CogPropertiesReaderMulti

+ (NSDictionary *)propertiesForSource:(id<CogSource>)source readers:(NSArray *)readers {
	NSArray *sortedReaders = sortClassesByPriority(readers);
	for(NSString *classString in sortedReaders) {
		Class reader = NSClassFromString(classString);
		NSDictionary *data = [reader propertiesForSource:source];
		if([data count])
			return data;
		if([source seekable])
			[source seek:0 whence:SEEK_SET];
	}
	return nil;
}

@end
