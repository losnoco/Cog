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
	
	BOOL malformed = NO;

	const int segmentCount = 4;
	const int lastSegment = segmentCount - 1;
	int segments[segmentCount] = {-1, -1, -1, -1};
	int lastScannedSegment = -1;

	BOOL isNegative = NO;
	
	if ([scanner isAtEnd] == NO) {
		isNegative = [scanner scanString:@"-" intoString:NULL];
		
		int segmentIndex = 0;
		
		while ([scanner isAtEnd] == NO) {
			// Grab a value
			if ([scanner scanInt:&(segments[segmentIndex])] == NO) {
				segments[segmentIndex] = -1;
				malformed = YES;
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
	
	if ([scanner isAtEnd] == NO) {
		malformed = YES;
	}
	
	int seconds = 0;
	
	if (malformed == NO) {
		int secondsIndex;
		int minutesIndex;
		int hoursIndex;
		int daysIndex;
		
		switch (lastScannedSegment) {
			case 0: {
				secondsIndex = 0;
				minutesIndex = -1;
				hoursIndex = -1;
				daysIndex = -1;
				break;
			}
				
			case 1: {
				secondsIndex = 1;
				minutesIndex = 0;
				hoursIndex = -1;
				daysIndex = -1;
				break;
			}
				
			case 2: {
				secondsIndex = 2;
				minutesIndex = 1;
				hoursIndex = 0;
				daysIndex = -1;
				break;
			}
				
			case 3: {
				secondsIndex = 3;
				minutesIndex = 2;
				hoursIndex = 1;
				daysIndex = 0;
				break;
			}
				
			default: {
				secondsIndex = -1;
				minutesIndex = -1;
				hoursIndex = -1;
				daysIndex = -1;
				break;
			}
		}
		
		const BOOL hasDaysSegment = daysIndex >= 0;
		const BOOL hasHoursSegment = hoursIndex >= 0;
		const BOOL hasMinutesSegment = minutesIndex >= 0;
		const BOOL hasSecondsSegment = secondsIndex >= 0;
		
		if (hasDaysSegment) {
			if ((segments[daysIndex] >= 0) && (segments[daysIndex] < INT32_MAX)) {
				seconds += segments[daysIndex];
				seconds *= 24;
			}
			else {
				malformed = YES;
			}
		}

		if (hasHoursSegment) {
			if ((segments[hoursIndex] >= 0) && (segments[hoursIndex] < 24)) {
				seconds += segments[hoursIndex];
				seconds *= 60;
			}
			else {
				malformed = YES;
			}
		}
		
		if (hasMinutesSegment) {
			if ((segments[minutesIndex] >= 0) && (segments[minutesIndex] < 60)) {
				seconds += segments[minutesIndex];
				seconds *= 60;
			}
			else {
				malformed = YES;
			}
		}
		
		if (hasSecondsSegment) {
			if ((segments[secondsIndex] >= 0) && (segments[secondsIndex] < 60)) {
				seconds += segments[secondsIndex];
			}
			else {
				malformed = YES;
			}
		}
		else {
			malformed = YES;
		}
		
		seconds *= (isNegative ? -1 : 1);
	}
	
	const BOOL result = (malformed == NO);
	
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
