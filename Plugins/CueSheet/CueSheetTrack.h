//
//  CueSheetTrack.h
//  CueSheet
//
//  Created by Zaphod Beeblebrox on 10/8/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface CueSheetTrack : NSObject {
	NSString *track;
	NSURL *url;

	NSString *artist;
	NSString *album;
	NSString *title;
	NSString *genre;
	NSString *year;

	float albumGain;
	float albumPeak;
	float trackGain;
	float trackPeak;

	double time; // Starting time for the track
	BOOL timeInSamples;
}

+ (id)trackWithURL:(NSURL *)u track:(NSString *)t time:(double)s timeInSamples:(BOOL)tis artist:(NSString *)a album:(NSString *)b title:(NSString *)l genre:(NSString *)g year:(NSString *)y albumGain:(float)albumGain albumPeak:(float)albumPeak trackGain:(float)trackGain trackPeak:(float)trackPeak;
- (id)initWithURL:(NSURL *)u track:(NSString *)t time:(double)s timeInSamples:(BOOL)tis artist:(NSString *)a album:(NSString *)b title:(NSString *)l genre:(NSString *)g year:(NSString *)y albumGain:(float)albumGain albumPeak:(float)albumPeak trackGain:(float)trackGain trackPeak:(float)trackPeak;

- (NSString *)track;
- (NSURL *)url;
- (NSString *)artist;
- (NSString *)album;
- (NSString *)title;
- (NSString *)genre;
- (NSString *)year;

- (float)albumGain;
- (float)albumPeak;
- (float)trackGain;
- (float)trackPeak;

- (double)time;
- (BOOL)timeInSamples;

@end
