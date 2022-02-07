//
//  AdPlugContainer.m
//  AdPlug
//
//  Created by Christopher Snowhill on 1/27/18.
//  Copyright 2018 __LoSnoCo__. All rights reserved.
//

#import <libAdPlug/adplug.h>
#import <libAdPlug/silentopl.h>

#import "AdPlugContainer.h"
#import "AdPlugDecoder.h"

#import "Logging.h"

#import "fileprovider.h"

@implementation AdPlugContainer

+ (NSArray *)fileTypes {
	return [AdPlugDecoder fileTypes];
}

+ (NSArray *)mimeTypes {
	return nil;
}

+ (float)priority {
	return [AdPlugDecoder priority];
}

+ (NSArray *)urlsForContainerURL:(NSURL *)url {
	if([url fragment]) {
		// input url already has fragment defined - no need to expand further
		return [NSMutableArray arrayWithObject:url];
	}

	id audioSourceClass = NSClassFromString(@"AudioSource");
	id<CogSource> source = [audioSourceClass audioSourceForURL:url];

	if(![source open:url])
		return 0;

	if(![source seekable])
		return 0;

	Copl *p_emu = new CSilentopl;

	std::string path = [[[url absoluteString] stringByRemovingPercentEncoding] UTF8String];
	CPlayer *p_player = CAdPlug::factory(path, p_emu, CAdPlug::players, CProvider_cog(path, source));

	if(!p_player) {
		delete p_emu;
		return 0;
	}

	NSMutableArray *tracks = [NSMutableArray array];

	unsigned int i;
	unsigned int subsongs = p_player->getsubsongs();

	delete p_player;
	delete p_emu;

	for(i = 0; i < subsongs; ++i) {
		[tracks addObject:[NSURL URLWithString:[[url absoluteString] stringByAppendingFormat:@"#%i", i]]];
	}

	return tracks;
}

@end
