/*
 *  $Id: SecondsFormatter.m 227 2007-01-21 06:22:13Z stephen_booth $
 *
 *  Copyright (C) 2006 - 2007 Stephen F. Booth <me@sbooth.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#import "SecondsFormatter.h"

@implementation SecondsFormatter

- (NSString *) stringForObjectValue:(id)object
{
	if (nil == object || NO == [object isKindOfClass:[NSNumber class]]) {
		// Docs state: “Returns nil if object is not of the correct class.”
		return nil;
	}
	
	double floatValue = [object doubleValue];
	
	if (isnan(floatValue)) { return @"NaN"; }
	if (isinf(floatValue)) { return @"Inf"; }
	
	int totalSeconds		= (int)floatValue;
	
	int seconds	= totalSeconds % 60;
	int minutes	= totalSeconds / 60;
	int hours = 0;
	int days = 0;
	
	while(60 <= minutes) {
		minutes -= 60;
		++hours;
	}
	
	while(24 <= hours) {
		hours -= 24;
		++days;
	}
	
	NSString *result = nil;
	
	if(0 < days) {
		result = [NSString stringWithFormat:@"%i:%.2i:%.2i:%.2i", days, hours, minutes, seconds];
	}
	else if(0 < hours) {
		result = [NSString stringWithFormat:@"%i:%.2i:%.2i", hours, minutes, seconds];
	}
	else if(0 < minutes) {
		result = [NSString stringWithFormat:@"%i:%.2i", minutes, seconds];
	}
	else {
		result = [NSString stringWithFormat:@"0:%.2i", seconds];
	}
	
	return result;
}

- (BOOL)getObjectValue:(out id  _Nullable __autoreleasing *)object forString:(NSString *)string errorDescription:(out NSString * _Nullable __autoreleasing *)error
{
	NSScanner		*scanner		= nil;
	BOOL			result			= NO;
	int				value			= 0;
	int				seconds			= 0;
	
	scanner		= [NSScanner scannerWithString:string];
	
	while(NO == [scanner isAtEnd]) {
		
		// Grab a value
		if([scanner scanInt:&value]) {
			seconds		*= 60;
			seconds		+= value;
			result		= YES;
		}
		
		// Grab the separator, if present
		[scanner scanString:@":" intoString:NULL];
	}
	
	if(result && NULL != object) {
		*object = [NSNumber numberWithInt:seconds];
	}
	else if(NULL != error) {
		*error = @"Couldn't convert value to seconds";
	}
	
	return result;
}

- (NSAttributedString *) attributedStringForObjectValue:(id)object withDefaultAttributes:(NSDictionary *)attributes
{
	NSString *stringValue = [self stringForObjectValue:object];
	if(nil == stringValue)
		return nil;
	
	NSAttributedString *result = [[NSAttributedString alloc] initWithString:stringValue attributes:attributes];
	return result;
}

@end
