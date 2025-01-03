//
//  ContainedNode.m
//  Cog
//
//  Created by Vincent Spader on 10/15/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "ContainedNode.h"
#import "CogAudio/AudioMetadataReader.h"

@implementation ContainedNode

- (BOOL)isLeaf {
	return YES;
}

- (void)setURL:(NSURL *)u {
	[super setURL:u];

	if([u fragment]) {
		NSDictionary *metadata = [AudioMetadataReader metadataForURL:u];
		NSString *title = nil;
		NSString *artist = nil;
		if(metadata) {
			id _title = [metadata valueForKey:@"title"];
			id _artist = [metadata valueForKey:@"artist"];

			if([_title isKindOfClass:[NSArray class]]) {
				NSArray *titlearray = _title;
				title = [titlearray componentsJoinedByString:@", "];
			} else if([_title isKindOfClass:[NSString class]]) {
				title = _title;
			} else if([_title isKindOfClass:[NSNumber class]]) {
				NSNumber *titlenumber = _title;
				title = [NSString stringWithFormat:@"%@", titlenumber];
			}

			if([_artist isKindOfClass:[NSArray class]]) {
				NSArray *artistarray = _artist;
				artist = [artistarray componentsJoinedByString:@", "];
			} else if([_artist isKindOfClass:[NSString class]]) {
				artist = _artist;
			} else if([_artist isKindOfClass:[NSNumber class]]) {
				NSNumber *artistnumber = _artist;
				artist = [NSString stringWithFormat:@"%@", artistnumber];
			}
		}

		if(title && [title length]) {
			if(artist && [artist length]) {
				display = [[u fragment] stringByAppendingFormat:@": %@ - %@", artist, title];
			} else {
				display = [[u fragment] stringByAppendingFormat:@": %@", title];
			}
		} else {
			display = [u fragment];
		}
	}
}

@end
