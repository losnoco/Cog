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

	double time;
}

+ (id)trackWithURL:(NSURL *)u track:(NSString *)t time:(double)s artist:(NSString *)a album:(NSString *)b title:(NSString *)l genre:(NSString *)g year:(NSString *)y;
- (id)initWithURL:(NSURL *)u track:(NSString *)t time:(double)s artist:(NSString *)a album:(NSString *)b title:(NSString *)l genre:(NSString *)g year:(NSString *)y;


- (NSString *)track;
- (NSURL *)url;
- (NSString *)artist;
- (NSString *)album;
- (NSString *)title;
- (NSString *)genre;
- (NSString *)year;

- (double)time;

@end
