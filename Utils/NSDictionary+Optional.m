#import "NSDictionary+Merge.h"

@implementation NSDictionary (Optional)

+ (NSDictionary *)initWithOptionalObjects:(const id _Nonnull [_Nullable])objects forKeys:(id<NSCopying> const[])keys count:(NSUInteger)cnt {
	NSMutableDictionary *dictionary = [[NSMutableDictionary alloc] init];
	for(NSUInteger i = 0; i < cnt; ++i) {
		if(keys[i] && objects[i]) {
			[dictionary setObject:objects[i] forKey:keys[i]];
		}
	}
	return [NSDictionary dictionaryWithDictionary:dictionary];
}

@end
