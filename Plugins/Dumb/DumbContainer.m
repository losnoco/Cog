//
//  DumbContainer.m
//  Dumb
//
//  Created by Christopher Snowhill on 10/4/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#import <Dumb/dumb.h>

#import "DumbContainer.h"
#import "DumbDecoder.h"

@implementation DumbContainer

+ (NSArray *)fileTypes
{
    return [DumbDecoder fileTypes];
}

+ (NSArray *)mimeTypes 
{
	return nil;
}

struct callbackData
{
    NSString * baseUrl;
    NSMutableArray * tracks;
};

int scanCallback(void *data, int startOrder, long length)
{
    struct callbackData * cbData = ( struct callbackData * ) data;
    
    [cbData->tracks addObject:[NSURL URLWithString:[cbData->baseUrl stringByAppendingFormat:@"#%i", startOrder]]];
    
    return 0;
}

+ (NSArray *)urlsForContainerURL:(NSURL *)url
{
    id audioSourceClass = NSClassFromString(@"AudioSource");
    id<CogSource> source = [audioSourceClass audioSourceForURL:url];
    
    if (![source open:url])
        return 0;
    
    if (![source seekable])
        return 0;
	
	DUMBFILE * df = dumbfile_open_ex(source, &dfs);
	if (!df)
	{
		NSLog(@"EX Failed");
		return NO;
	}
    
    NSMutableArray *tracks = [NSMutableArray array];
    
	int i;
    int subsongs = dumb_get_psm_subsong_count( df );
    if ( subsongs ) {
        dumbfile_close( df );
        
        for (i = 0; i < subsongs; ++i) {
            [tracks addObject:[NSURL URLWithString:[[url absoluteString] stringByAppendingFormat:@"#%i", i]]];
        }
    } else {
        dumbfile_seek( df, 0, SEEK_SET );
        
        DUH *duh;
        NSString *ext = [[[url path] pathExtension] lowercaseString];
        duh = dumb_read_any_quick(df, [ext isEqualToString:@"mod"] ? 0 : 1, 0);

        dumbfile_close(df);

        if ( duh ) {
            struct callbackData data = { [url absoluteString], tracks };
            
            dumb_it_scan_for_playable_orders( duh_get_it_sigdata( duh ), scanCallback, &data );
        }
    }
    
	return tracks;
}


@end
