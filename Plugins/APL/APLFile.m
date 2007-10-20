//
//  APLFile.m
//  APL
//
//  Created by ??????? ???????? on 10/18/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "APLFile.h"
#import "ApeTag.h"

@implementation APLFile
+createWithFile:(NSString*)f { return [[APLFile alloc] initWithFile:f];	}


-(NSString*) readline:(NSFileHandle*)f { // rather hack-style substitution to fgets...
	NSMutableData* d = [NSMutableData dataWithCapacity:100]; //it will grow, here should be most expected value (may gain few nanosecond from it =) )
	while(true) {
		NSData* byte = [f readDataOfLength:1];
		if (!byte)
		{
			[f seekToFileOffset:([f offsetInFile]-[d length])];
			return nil;
		}
		[d appendData:byte];
		if (*((const char*)[byte bytes]) == '\n') break;
	}
	NSString* s = [[NSString alloc] initWithData:d encoding:NSUTF8StringEncoding]; //!handle encoding error (in case binary data starts not from newline, impossible on real apl, but who knows...)
	return s;
}

- (NSURL *)urlForPath:(NSString *)path relativeTo:(NSString *)baseFilename
{
	NSRange protocolRange = [path rangeOfString:@"://"];
	if (protocolRange.location != NSNotFound) 
	{
		return [NSURL URLWithString:path];
	}
	
	NSMutableString *unixPath = [path mutableCopy];
	
	NSString *fragment = @"";
	NSRange fragmentRange = [path rangeOfString:@"#"];
	if (fragmentRange.location != NSNotFound) 
	{
		fragmentRange = NSMakeRange(fragmentRange.location, [unixPath length] - fragmentRange.location);
		
		fragment = [unixPath substringWithRange:fragmentRange];
		[unixPath deleteCharactersInRange:fragmentRange];
	}
	
	if (![unixPath hasPrefix:@"/"]) {
		//Only relative paths would have windows backslashes.
		[unixPath replaceOccurrencesOfString:@"\\" withString:@"/" options:0 range:NSMakeRange(0, [unixPath length])];
		
		NSString *basePath = [[[baseFilename stringByStandardizingPath] stringByDeletingLastPathComponent] stringByAppendingString:@"/"];
		
		[unixPath insertString:basePath atIndex:0];
	}
	
	//Append the fragment
	return [NSURL URLWithString:[[[NSURL fileURLWithPath:unixPath] absoluteString] stringByAppendingString: fragment]];
}

-initWithFile:(NSString*)filename {
	self = [super init];
	if (self)
	{
		//startBlock must be always >= 0
		NSFileHandle* f = (NSFileHandle*)[NSFileHandle fileHandleForReadingAtPath:filename];
		if(!f){
			NSLog(@"Failed to open apl file '%@' for reading", f);
			return nil;
		}
		NSString* header = @"[Monkey's Audio Image Link File]\r\n";
		NSData* da = [f readDataOfLength:[header length]];
		if (!da) {
			NSLog(@"Cannot read header");
			return nil;
		}
		NSString* str = [[NSString alloc] initWithData:da encoding: NSASCIIStringEncoding];	
		if([str compare:header options:NSCaseInsensitiveSearch]) {
			NSLog(@"APL header mismatch");
			return nil;
		}
		//now read by lines, skip empty, up to line <tagline> (or any starting with '-' - may be other tags can be present)
		NSString* line = nil;
		NSScanner *scanner = nil;
		//NSCharacterSet *whitespace = [NSCharacterSet whitespaceAndNewlineCharacterSet];
		while(line = [self readline:f]) {
			if (![line compare:@"----- APE TAG (DO NOT TOUCH!!!) -----\r\n" options:NSCaseInsensitiveSearch]) break;
			if([line characterAtIndex:0] == '-') break;
			[scanner release];
			scanner = [[NSScanner alloc] initWithString:line];
			NSString* field = nil, *value = nil;
			if (![scanner scanUpToString:@"=" intoString:&field]) continue;
			if (![scanner scanString:@"=" intoString:nil]) continue;
			if (![scanner scanUpToString:@"\r\n" intoString:&value]) continue;
			if (![field compare:@"Image File" options:NSCaseInsensitiveSearch])
			{
				[file release];
				file = [self urlForPath:value relativeTo:filename];
				NSLog(@"APL refers to file '%@'", file);
				continue;
			}
			if (![field compare:@"Start Block" options:NSCaseInsensitiveSearch])
			{
				startBlock = [value intValue]; //!!! bugs with files over 2GB
				NSLog(@"APL start block %d (%@)", startBlock, value);
				continue;
			}
			if (![field compare:@"Finish Block" options:NSCaseInsensitiveSearch])
			{
				endBlock = [value intValue]; //!!! bugs with files over 2GB
				NSLog(@"APL start block %d (%@)", endBlock, value);
				continue;
			}
		}
		[scanner release];
		//check here for EOF? cocoa does not have this functionality :(
		tag = [ApeTag createFromFileRead:f];
		[f closeFile];
		}
	return self;
}

-(long)startBlock { return startBlock; }
-(long)endBlock { return endBlock; }
-(ApeTag*) tag { return tag; }
-(NSURL*) file { return file; }



@end
