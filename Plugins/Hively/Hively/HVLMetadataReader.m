//
//  HVLMetadataReader.m
//  Hively
//
//  Created by Christopher Snowhill on 10/29/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#import "HVLMetadataReader.h"
#import "HVLDecoder.h"

@implementation HVLMetadataReader

+ (NSArray *)fileTypes
{
	return [HVLDecoder fileTypes];
}

+ (NSArray *)mimeTypes
{
	return [HVLDecoder mimeTypes];
}

+ (float)priority
{
    return 1.0f;
}

+ (NSDictionary *)metadataForURL:(NSURL *)url
{
    id audioSourceClass = NSClassFromString(@"AudioSource");
    id<CogSource> source = [audioSourceClass audioSourceForURL:url];
    
    if (![source open:url])
        return 0;
    
    if (![source seekable])
        return 0;

    [source seek:0 whence:SEEK_END];
    long size = [source tell];
    [source seek:0 whence:SEEK_SET];
    
    if ( size > UINT_MAX )
        return nil;
    
    void * data = malloc(size);
    [source read:data amount:size];
	
    struct hvl_tune * tune = hvl_LoadTune( data, (uint32_t) size, 44100, 2 );
    free( data );
    if ( !tune )
        return nil;

	NSString *title = [[NSString stringWithUTF8String: tune->ht_Name] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];

    hvl_FreeTune( tune );

	if (title == nil) {
		title = @"";
	}
	
	return [NSDictionary dictionaryWithObject:title forKey:@"title"];
}

@end
