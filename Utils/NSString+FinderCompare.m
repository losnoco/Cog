//
//  FinderCompare.m
//  Created by Pablo Gomez Basanta on 23/7/05.
//  From: http://neop.gbtopia.com/?p=27
//
//  Based on:
//  http://developer.apple.com/qa/qa2004/qa1159.html
//

#import "NSString+FinderCompare.h"



@implementation NSString (FinderCompare)

- (NSComparisonResult)finderCompare:(NSString *)aString
{
	SInt32 compareResult;
	
	CFIndex lhsLen = [self length];;
    CFIndex rhsLen = [aString length];
	
	UniChar *lhsBuf = malloc(lhsLen * sizeof(UniChar));
	UniChar *rhsBuf = malloc(rhsLen * sizeof(UniChar));
	
	[self getCharacters:lhsBuf];
	[aString getCharacters:rhsBuf];
	
	(void) UCCompareTextDefault(kUCCollateComposeInsensitiveMask | kUCCollateWidthInsensitiveMask | kUCCollateCaseInsensitiveMask | kUCCollateDigitsOverrideMask | kUCCollateDigitsAsNumberMask| kUCCollatePunctuationSignificantMask,lhsBuf,lhsLen,rhsBuf,rhsLen,NULL,&compareResult);
	
	free(lhsBuf);
	free(rhsBuf);
	
	return (NSComparisonResult) compareResult;
}

@end

@implementation NSURL (FinderCompare)

- (NSComparisonResult)finderCompare:(NSURL *)aURL
{
	return [[self absoluteString] finderCompare:[aURL absoluteString]];
}

@end
