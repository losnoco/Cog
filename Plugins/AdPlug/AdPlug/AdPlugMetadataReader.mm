//
//  AdPlugMetadataReader.m
//  AdPlug
//
//  Created by Christopher Snowhill on 1/27/18.
//  Copyright 2018 __LoSnoCo__. All rights reserved.
//

#import "AdPlugMetadataReader.h"
#import "AdPlugDecoder.h"

#import <AdPlug/adplug.h>
#import <AdPlug/silentopl.h>

#import "fileprovider.h"

#import "Logging.H"

@implementation AdPlugMetadataReader

+ (NSArray *)fileTypes
{
	return [AdPlugDecoder fileTypes];
}

+ (NSArray *)mimeTypes
{
	return [AdPlugDecoder mimeTypes];
}

+ (float)priority
{
    return [AdPlugDecoder priority];
}

+ (NSDictionary *)metadataForURL:(NSURL *)url
{
    id audioSourceClass = NSClassFromString(@"AudioSource");
    id<CogSource> source = [audioSourceClass audioSourceForURL:url];
    
    if (![source open:url])
        return 0;
    
    if (![source seekable])
        return 0;

    Copl * p_emu = new CSilentopl;
    
    std::string path = [[[url absoluteString] stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding] UTF8String];
    CPlayer * p_player = CAdPlug::factory(path, p_emu, CAdPlug::players, CProvider_cog( path, source ));
    
    if ( !p_player )
    {
        delete p_emu;
        return 0;
    }

    NSString * title = @"";
    NSString * artist = @"";
    
    if ( !p_player->gettitle().empty() )
        title = [NSString stringWithUTF8String: p_player->gettitle().c_str()];
    if ( !p_player->getauthor().empty() )
        artist = [NSString stringWithUTF8String: p_player->getauthor().c_str()];

    delete p_player;
    delete p_emu;

    return [NSDictionary dictionaryWithObjectsAndKeys:title, @"title", artist, @"artist", nil];
}

@end
