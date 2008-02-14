//
//  NSComparisonPredicate+CogPredicate.m
//  Cog
//
//  Created by Matthew Grinshpun on 14/02/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "NSComparisonPredicate+CogPredicate.h"

// Ignore case and diacritics
static const unsigned OPTIONS = (NSCaseInsensitivePredicateOption|
                                 NSDiacriticInsensitivePredicateOption);

@implementation NSComparisonPredicate (CogPredicate)

+ (NSPredicate*)predicateForMdKey:(NSString *)key
                       withString:(NSString *)aString
                      exactString:(BOOL)exactString
{
    // We don't want an exact string, so wrap it in wildcards
    if(!exactString)
        aString = [NSString stringWithFormat:@"*%@*", aString];
        
    return [NSComparisonPredicate
        predicateWithLeftExpression:[NSExpression expressionForKeyPath:key]
                    rightExpression:[NSExpression expressionForConstantValue:aString]
                           modifier:NSDirectPredicateModifier
                               type:NSLikePredicateOperatorType
                            options:OPTIONS];
}

@end