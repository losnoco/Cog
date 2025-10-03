#import "NSDictionary+Merge.h"

@implementation NSDictionary (Optional)

+ (NSDictionary *_Nonnull)initWithOptionalObjects:(const id _Nullable [_Nullable])objects forKeys:(id<NSCopying> const _Nullable [_Nullable])keys count:(NSUInteger)cnt {
	NSMutableDictionary *dictionary = [NSMutableDictionary new];
	if(keys && objects && cnt) {
		for(NSUInteger i = 0; i < cnt; ++i) {
			if(keys[i] && objects[i]) {
				[dictionary setObject:objects[i] forKey:keys[i]];
			}
		}
	}
	return [NSDictionary dictionaryWithDictionary:dictionary];
}

@end
