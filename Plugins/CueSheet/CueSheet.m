//
//  CueSheet.m
//  CueSheet
//
//  Created by Zaphod Beeblebrox on 10/8/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "CueSheet.h"
#import "CueSheetTrack.h"

@implementation CueSheet

+ (id)cueSheetWithFile:(NSString *)filename
{
	return [[[CueSheet alloc] initWithFile:filename] autorelease];
}

- (id)initWithFile:(NSString *)filename
{
	self = [super init];
	if (self) {
		
		NSError *error = nil;
		NSString *contents = [NSString stringWithContentsOfFile:filename encoding:NSUTF8StringEncoding error:&error];
		if (error || !contents) {
			NSLog(@"Could not open file...%@ %@", contents, error);
			return nil;
		}
		
		tracks = [[NSMutableArray alloc] init];
		
		//Actually parse file here.
	}
	
	return self;
}


- (NSArray *)tracks
{
	return tracks;
}

- (CueSheetTrack *)track:(NSString *)fragment
{
	CueSheetTrack *t;
	NSEnumerator *e = [tracks objectEnumerator];
	while (t = [e nextObject]) {
		if ([[t track] isEqualToString:fragment]) {
			return t;
		}
	}
	
	return nil;
}

@end
