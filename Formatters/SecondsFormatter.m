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
	
	BOOL isNegative = floatValue < 0;

	int totalSeconds = (int)(isNegative ? -floatValue : floatValue);
	
	int seconds	= totalSeconds % 60;
	int minutes	= totalSeconds / 60;
	int hours = 0;
	int days = 0;
	
	while (60 <= minutes) {
		minutes -= 60;
		++hours;
	}
	
	while (24 <= hours) {
		hours -= 24;
		++days;
	}
	
	NSString *result = nil;
	
	const char *signPrefix = isNegative ? "-" : "";
	
	if (0 < days) {
		result = [NSString stringWithFormat:@"%s%i:%.2i:%.2i:%.2i", signPrefix, days, hours, minutes, seconds];
	}
	else if (0 < hours) {
		result = [NSString stringWithFormat:@"%s%i:%.2i:%.2i", signPrefix, hours, minutes, seconds];
	}
	else if (0 < minutes) {
		result = [NSString stringWithFormat:@"%s%i:%.2i", signPrefix, minutes, seconds];
	}
	else {
		result = [NSString stringWithFormat:@"%s0:%.2i", signPrefix, seconds];
	}
	
	return result;
}

- (BOOL)getObjectValue:(out id  _Nullable __autoreleasing *)object
			 forString:(NSString *)string
	  errorDescription:(out NSString * _Nullable __autoreleasing *)error
{
	NSScanner *scanner = [NSScanner scannerWithString:string];
	
	BOOL result = NO;
	
	const int segmentCount = 4;
	const int lastSegment = segmentCount - 1;
	int segments[segmentCount] = {-1, -1, -1, -1};
	int lastScannedSegment = -1;

	BOOL isNegative = NO;
	
	if ([scanner isAtEnd] == NO) {
		isNegative = [scanner scanString:@"-" intoString:NULL];
		
		int segmentIndex = 0;
		
		while (NO == [scanner isAtEnd]) {
			// Grab a value
			if ([scanner scanInt:&(segments[segmentIndex])] == NO) {
				segments[segmentIndex] = -1;
				break;
			}
			
			if (segmentIndex == lastSegment) {
				break;
			}
			
			// Grab the separator, if present
			if ([scanner scanString:@":" intoString:NULL] == NO) {
				break;
			}
			
			segmentIndex += 1;
		}
		
		lastScannedSegment = segmentIndex;
	}
	
	int seconds = 0;
	
	const BOOL hasDaysSegment = (lastScannedSegment == 3);
	const BOOL hasHoursSegment = (lastScannedSegment >= 2);

	for (int i = 0; i <= lastScannedSegment; i += 1) {
		if (segments[i] < 0) {
			break;
		}
		
		if (hasDaysSegment &&
			(i == 1)) {
			// Special case for days.
			seconds *= 24;
		}
		else {
			seconds *= 60;
		}
		
		const BOOL isDaysSegment = (hasDaysSegment && (i == 0));
		const BOOL isHoursSegment = (hasHoursSegment && (((lastScannedSegment == 3) && (i == 1)) || ((lastScannedSegment == 2) && (i == 0))));

		if (isDaysSegment ||
			((isDaysSegment == NO) &&
			 ((isHoursSegment && (segments[i] < 24)) ||
			  ((isHoursSegment == NO) &&
			   (segments[i] < 60))))) {
			seconds += segments[i];
		}
		else {
			result = NO;
			break;
		}
		
		if (i == 0) {
			result = YES;
		}
	}
	
	if (isNegative) { seconds *= -1; }

	if (result && NULL != object) {
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
