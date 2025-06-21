#import <Foundation/Foundation.h>

@interface NSDictionary (Optional)

+ (NSDictionary *)initWithOptionalObjects:(const id _Nonnull [_Nullable])objects forKeys:(id<NSCopying> const[])keys count:(NSUInteger)cnt;

@end
