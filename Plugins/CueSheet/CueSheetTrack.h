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

	double start;
	double end;
}

- (void)initWithTrack:(NSString *)t start:(double)s end:(double)e;

- (NSString *)track;

- (double)start;
- (double)end;

@end
