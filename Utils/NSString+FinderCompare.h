/*
 *  NSString+FinderCompare.h
 */

#import <Cocoa/Cocoa.h>
#include <CoreServices/CoreServices.h>

@interface NSString (FinderCompare)

- (NSComparisonResult)finderCompare:(NSString *)aString;

@end
