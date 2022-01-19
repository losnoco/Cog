//
//  MIDIContainer.mm
//  MIDI
//
//  Created by Christopher Snowhill on 10/16/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#import "MIDIContainer.h"
#import "MIDIDecoder.h"

#import <midi_processing/midi_processor.h>

@implementation MIDIContainer

+ (NSArray *)fileTypes
{
    return [MIDIDecoder fileTypes];
}

+ (NSArray *)mimeTypes 
{
	return [MIDIDecoder mimeTypes];
}

+ (float) priority
{
    return 1.0f;
}

//This really should be source...
+ (NSArray *)urlsForContainerURL:(NSURL *)url
{
    if ([url fragment]) {
        // input url already has fragment defined - no need to expand further
        return [NSMutableArray arrayWithObject:url];
    }
    
    id audioSourceClass = NSClassFromString(@"AudioSource");
    id<CogSource> source = [audioSourceClass audioSourceForURL:url];
    
    if (![source open:url])
        return @[];
    
    if (![source seekable])
        return @[];
	
    [source seek:0 whence:SEEK_END];
    long size = [source tell];
    [source seek:0 whence:SEEK_SET];
    
    std::vector<uint8_t> data;
    data.resize( size );
    [source read:&data[0] amount:size];
    
    size_t track_count = 0;
    
    if ( !midi_processor::process_track_count( data, [[url pathExtension] UTF8String], track_count) )
        return @[];

	NSMutableArray *tracks = [NSMutableArray array];
	
	long i;
	for (i = 0; i < track_count; i++) {
		[tracks addObject:[NSURL URLWithString:[[url absoluteString] stringByAppendingFormat:@"#%li", i]]];
	}
	
	return tracks;
}


@end
