//
//  TagLibMetadataWriter.m
//  TagLib
//
//  Created by Safari on 08/11/17.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "TagLibMetadataWriter.h"

#import <tag/fileref.h>
#import <tag/tag.h>
#import <tag/tpropertymap.h>

#import "Logging.h"

static TagLib::StringList toArray(id value) {
	TagLib::StringList newvalues;
	if([value isKindOfClass:[NSString class]]) {
		NSString *stringValue = (NSString *)value;
		newvalues.append([stringValue UTF8String]);
	} else if([value isKindOfClass:[NSNumber class]]) {
		NSNumber *numberValue = (NSNumber *)value;
		newvalues.append([[numberValue stringValue] UTF8String]);
	} else if([value isKindOfClass:[NSArray class]]) {
		NSArray *arrayValue = (NSArray *)value;
		for(id subvalue : arrayValue) {
			if([subvalue isKindOfClass:[NSString class]]) {
				NSString *stringValue = (NSString *)subvalue;
				newvalues.append([stringValue UTF8String]);
			} else if([subvalue isKindOfClass:[NSNumber class]]) {
				NSNumber *numberValue = (NSNumber *)subvalue;
				newvalues.append([[numberValue stringValue] UTF8String]);
			}
		}
	}
	return newvalues;
}

@implementation TagLibMetadataWriter

+ (int)putMetadataInURL:(NSURL *)url tagData:(NSDictionary *)tagData {
	NSMutableArray *tagFound = [NSMutableArray new];
	try {
		TagLib::FileRef f((const char *)[[url path] UTF8String], false);
		if(!f.isNull()) {
			NSDictionary *tagmap = @{@"tracknumber": @"track",
									 @"discnumber": @"disc",
									 @"itunnorm": @"soundcheck"
			};
			NSDictionary *reversemap = @{@"track": @"TRACKNUMBER",
										 @"disc": @"DISCNUMBER",
										 @"soundcheck": @"ITUNNORM"
			};

			__block TagLib::PropertyMap map = f.properties();

			for(auto i = map.cbegin(); i != map.cend(); ++i) {
				NSString *name = [guess_encoding_of_string(i->first.toCString(true)) lowercaseString];
				NSString *mappedname = [tagmap valueForKey:name];
				id value;
				if(!mappedname) {
					mappedname = name;
				}
				value = [tagData valueForKey:mappedname];
				if(value) {
					map.replace(i->first, toArray(value));
					[tagFound addObject:mappedname];
				}
			}

			[tagData enumerateKeysAndObjectsUsingBlock:^(id  _Nonnull key, id  _Nonnull obj, BOOL * _Nonnull stop) {
				if(![tagFound containsObject:key]) {
					NSString *mappedname = [reversemap valueForKey:key];
					if(!mappedname) {
						mappedname = [key uppercaseString];
					}
					map.insert([mappedname UTF8String], toArray(obj));
				}
			}];

			f.save();
		}
	} catch (std::exception &e) {
		ALog(@"Exception caught writing with TagLib: %s", e.what());
		return -1;
	}

	return 0;
}

@end
