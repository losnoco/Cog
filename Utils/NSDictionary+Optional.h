#import <Foundation/Foundation.h>

@interface NSDictionary (Optional)

+ (NSDictionary *_Nonnull)initWithOptionalObjects:(const id _Nullable [_Nullable])objects forKeys:(id<NSCopying> const _Nullable [_Nullable])keys count:(NSUInteger)cnt;

@end
