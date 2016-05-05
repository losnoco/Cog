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

- (BOOL)isLeaf
{
	return YES;
}

- (void)setURL:(NSURL *)u
{
	[super setURL:u];
	
	if ([u fragment])
	{
        NSDictionary *metadata = [AudioMetadataReader metadataForURL:u];
        NSString *title = nil;
        NSString *artist = nil;
        if (metadata)
        {
            title = [metadata valueForKey:@"title"];
            artist = [metadata valueForKey:@"artist"];
        }
        
        if (title && [title length])
        {
            if (artist && [artist length]) { display = [[u fragment] stringByAppendingFormat:@": %@ - %@", artist, title];}
            else { display = [[u fragment] stringByAppendingFormat:@": %@", title]; }
        }
        else
        {
            display = [u fragment];
        }
	}
}

@end
