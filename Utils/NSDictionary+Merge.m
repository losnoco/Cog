#import "NSDictionary+Merge.h"

@implementation NSDictionary (Merge)

+ (NSDictionary *)dictionaryByMerging:(NSDictionary *)dict1 with:(NSDictionary *)dict2 {
	NSMutableDictionary *result = [dict1 mutableCopy];

	[dict2 enumerateKeysAndObjectsUsingBlock:^(id key, id obj, BOOL *stop) {
		if(![dict1 objectForKey:key]) {
			[result setObject:obj forKey:key];
		} else if([obj isKindOfClass:[NSDictionary class]]) {
			NSDictionary *newVal = [[dict1 objectForKey:key] dictionaryByMergingWith:(NSDictionary *)obj];
			[result setObject:newVal forKey:key];
		} else {
			BOOL isEmpty = NO;
			id objTarget = [dict1 objectForKey:key];
			if([objTarget isKindOfClass:[NSString class]]) {
				NSString *val = (NSString *)objTarget;
				isEmpty = [val length] == 0;
			} else if([objTarget isKindOfClass:[NSNumber class]]) {
				NSNumber *val = (NSNumber *)objTarget;
				isEmpty = [val isEqualTo:@(0)];
			} else if([objTarget isKindOfClass:[NSData class]]) {
				NSData *val = (NSData *)objTarget;
				isEmpty = [val length] == 0;
			}
			if(isEmpty) {
				[result setObject:obj forKey:key];
			}
		}
	}];

	return (NSDictionary *)[NSDictionary dictionaryWithDictionary:result];
}

- (NSDictionary *)dictionaryByMergingWith:(NSDictionary *)dict {
	return [[self class] dictionaryByMerging:self with:dict];
}

@end
