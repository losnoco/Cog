//
//  CueSheetTrack.m
//  CueSheet
//
//  Created by Zaphod Beeblebrox on 10/8/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "CueSheetTrack.h"

@implementation CueSheetTrack

+ (id)trackWithURL:(NSURL *)u track:(NSString *)t time:(double)s timeInSamples:(BOOL)tis artist:(NSString *)a album:(NSString *)b title:(NSString *)l genre:(NSString *)g year:(NSString *)y albumGain:(float)albumGain albumPeak:(float)albumPeak trackGain:(float)trackGain trackPeak:(float)trackPeak {
	return [[CueSheetTrack alloc] initWithURL:u track:t time:s timeInSamples:tis artist:a album:b title:l genre:g year:y albumGain:albumGain albumPeak:albumPeak trackGain:trackGain trackPeak:trackPeak];
}

- (id)initWithURL:(NSURL *)u track:(NSString *)t time:(double)s timeInSamples:(BOOL)tis artist:(NSString *)a album:(NSString *)b title:(NSString *)l genre:(NSString *)g year:(NSString *)y albumGain:(float)albumGain albumPeak:(float)albumPeak trackGain:(float)trackGain trackPeak:(float)trackPeak {
	self = [super init];
	if(self) {
		track = [t copy];
		url = [u copy];
		artist = [a copy];
		album = [b copy];
		title = [l copy];
		genre = [g copy];
		year = [y copy];

		time = s;
		timeInSamples = tis;

		self->albumGain = albumGain;
		self->albumPeak = albumPeak;
		self->trackGain = trackGain;
		self->trackPeak = trackPeak;
	}

	return self;
}

- (NSString *)track {
	return track;
}

- (NSURL *)url {
	return url;
}

- (double)time {
	return time;
}

- (BOOL)timeInSamples {
	return timeInSamples;
}

- (NSString *)artist {
	return artist;
}

- (NSString *)album {
	return album;
}

- (NSString *)title {
	return title;
}

- (NSString *)genre {
	return genre;
}

- (NSString *)year {
	return year;
}

- (float)albumGain {
	return albumGain;
}

- (float)albumPeak {
	return albumPeak;
}

- (float)trackGain {
	return trackGain;
}

- (float)trackPeak {
	return trackPeak;
}

- (NSString *)description {
	return [NSString stringWithFormat:@"CueSheetTrack{track: %@,  url: %@, artist: %@, album: %@, title: %@, genre: %@, year: %@}", track, [url absoluteURL], artist, album, title, genre, year];
}

@end
