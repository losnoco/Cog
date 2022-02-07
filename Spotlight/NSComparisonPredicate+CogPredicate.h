//
//  NSComparisonPredicate+CogPredicate.h
//  Cog
//
//  Created by Matthew Grinshpun on 14/02/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface NSComparisonPredicate (CogPredicate)

+ (NSPredicate*)predicateForMdKey:(NSString*)key
                       withString:(NSString*)aString
                      exactString:(BOOL)exactString;

@end
