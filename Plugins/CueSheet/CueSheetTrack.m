//
//  CueSheetTrack.m
//  CueSheet
//
//  Created by Zaphod Beeblebrox on 10/8/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "CueSheetTrack.h"


@implementation CueSheetTrack

+ (id)trackWithURL:(NSURL *)u track:(NSString *)t time:(double)s artist:(NSString *)a album:(NSString *)b title:(NSString *)l genre:(NSString *)g year:(NSString *)y
{
	return [[[CueSheetTrack alloc] initWithURL:u track:t time:s artist:a album:b title:l genre:g year:y] autorelease];
}

- (id)initWithURL:(NSURL *)u track:(NSString *)t time:(double)s artist:(NSString *)a album:(NSString *)b title:(NSString *)l genre:(NSString *)g year:(NSString *)y
{
	self = [super init];
	if (self)
	{
		track = [t copy];
		url = [u copy];
		artist = [a copy];
		album = [b copy];
		title = [l copy];
		genre = [g copy];
		year = [y copy];
		
		time = s;
	}
	
	return self;
}

- (void)dealloc
{
	[track release];
	[url release];
	[artist release];
	[album release];
	[title release];
	[genre release];
	[year release];
	
	[super dealloc];
}

- (NSString *)track
{
	return track;
}

- (NSURL *)url
{
	return url;
}

- (double)time
{
	return time;
}

- (NSString *)artist
{
	return artist;
}

- (NSString *)album
{
	return album;
}

- (NSString *)title
{
	return title;
}

- (NSString *)genre
{
	return genre;
}

- (NSString *)year
{
	return year;
}


@end
