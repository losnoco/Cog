//
//  MonospacedDigitTextFieldCell.m
//  Cog
//
//  Created by Jan on 04.05.21.
//

#import "MonospacedDigitTextFieldCell.h"

@implementation MonospacedDigitTextFieldCell

- (void)setFont:(NSFont *)font
{
	CGFloat size = font.pointSize;
	
	NSNumber *weightNum = [font.fontDescriptor objectForKey:NSFontWeightTrait];
	NSFontWeight weight = weightNum ? weightNum.doubleValue : NSFontWeightRegular;
	
	super.font = [NSFont monospacedDigitSystemFontOfSize:size
												  weight:weight];
}

@end
