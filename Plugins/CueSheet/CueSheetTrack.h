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

	double time;
}

+ (id)trackWithURL:(NSURL *)u track:(NSString *)t time:(double)t;
- (id)initWithURL:(NSURL *)u track:(NSString *)t time:(double)t;


- (NSString *)track;
- (NSURL *)url;

- (double)time;

@end
