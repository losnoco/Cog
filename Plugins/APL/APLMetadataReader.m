//
//  APLMetadataReader.m


#import "APLMetadataReader.h"
#import "APLDecoder.h"

#import "APLFile.h"
#import "ApeTag.h"

@implementation APLMetadataReader

+ (NSArray *)fileTypes { return [APLDecoder fileTypes]; }
+ (NSArray *)mimeTypes { return [APLDecoder mimeTypes]; }

+ (NSDictionary *)metadataForURL:(NSURL *)url {
	if (![url isFileURL]) return nil;
	
	APLFile *apl = [APLFile createWithFile:[url path]];
	
	ApeTag* tag = [apl tag];
	NSDictionary* res =[tag convertToCogTag];
	return res;
}

@end
