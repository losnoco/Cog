
// as simple as it can be - just write a readable tag, no options etc.


#import "ApeTag.h"

#define CURRENT_APE_TAG_VERSION                 2000
/*****************************************************************************************
"Standard" APE tag fields
*****************************************************************************************/
#define APE_TAG_FIELD_TITLE                     @"Title"
#define APE_TAG_FIELD_ARTIST                    @"Artist"
#define APE_TAG_FIELD_ALBUM                     @"Album"
#define APE_TAG_FIELD_COMMENT                   @"Comment"
#define APE_TAG_FIELD_YEAR                      @"Year"
#define APE_TAG_FIELD_TRACK                     @"Track"
#define APE_TAG_FIELD_GENRE                     @"Genre"
#define APE_TAG_FIELD_COVER_ART_FRONT           @"Cover Art (front)"
#define APE_TAG_FIELD_NOTES                     @"Notes"
#define APE_TAG_FIELD_LYRICS                    @"Lyrics"
#define APE_TAG_FIELD_COPYRIGHT                 @"Copyright"
#define APE_TAG_FIELD_BUY_URL                   @"Buy URL"
#define APE_TAG_FIELD_ARTIST_URL                @"Artist URL"
#define APE_TAG_FIELD_PUBLISHER_URL             @"Publisher URL"
#define APE_TAG_FIELD_FILE_URL                  @"File URL"
#define APE_TAG_FIELD_COPYRIGHT_URL             @"Copyright URL"
#define APE_TAG_FIELD_MJ_METADATA               @"Media Jukebox Metadata"
#define APE_TAG_FIELD_TOOL_NAME                 @"Tool Name"
#define APE_TAG_FIELD_TOOL_VERSION              @"Tool Version"
#define APE_TAG_FIELD_PEAK_LEVEL                @"Peak Level"
#define APE_TAG_FIELD_REPLAY_GAIN_RADIO         @"Replay Gain (radio)"
#define APE_TAG_FIELD_REPLAY_GAIN_ALBUM         @"Replay Gain (album)"
#define APE_TAG_FIELD_COMPOSER                  @"Composer"
#define APE_TAG_FIELD_KEYWORDS                  @"Keywords"

/*****************************************************************************************
Standard APE tag field values
*****************************************************************************************/
#define APE_TAG_GENRE_UNDEFINED                 @"Undefined"
/*****************************************************************************************
Footer (and header) flags
*****************************************************************************************/
#define APE_TAG_FLAG_CONTAINS_HEADER            (1 << 31)
#define APE_TAG_FLAG_CONTAINS_FOOTER            (1 << 30)
#define APE_TAG_FLAG_IS_HEADER                  (1 << 29)

#define APE_TAG_FLAGS_DEFAULT                   (APE_TAG_FLAG_CONTAINS_FOOTER)

/*****************************************************************************************
Tag field flags
*****************************************************************************************/
#define TAG_FIELD_FLAG_READ_ONLY                (1 << 0)

#define TAG_FIELD_FLAG_DATA_TYPE_MASK           (6)
#define TAG_FIELD_FLAG_DATA_TYPE_TEXT_UTF8      (0 << 1)
#define TAG_FIELD_FLAG_DATA_TYPE_BINARY         (1 << 1)
#define TAG_FIELD_FLAG_DATA_TYPE_EXTERNAL_INFO  (2 << 1)
#define TAG_FIELD_FLAG_DATA_TYPE_RESERVED       (3 << 1)

/*****************************************************************************************
The footer at the end of APE tagged files (can also optionally be at the front of the tag)
*****************************************************************************************/
#define APE_TAG_FOOTER_BYTES    32

//------------------------

@implementation ApeTagItem
+createTag:(NSString*)t flags:(SInt32)f data:(NSData*)d{
	return [[ApeTagItem alloc] initTag:t flags:f data:d];
}

-initTag:(NSString*)t flags:(SInt32)f data:(NSData*)d{
	self = [super init];
	if (self)
	{
		tag = [t copy];
		data = [d copy];
		flags = f;
	}
	return self;
}

-(NSData*) pack {
	//item header:
	NSMutableData* d = [NSMutableData dataWithCapacity:8];
	UInt32 len = CFSwapInt32HostToLittle([data length]);
	UInt32 flags1 = CFSwapInt32HostToLittle(flags); // ApeTagv1 does not use this, 0 for v2 means only footer and all fields text in utf8 - just right what we want
	
	[d appendBytes:&len length:4]; // lenth of value
	[d appendBytes:&flags1 length:4]; // 32 bits of flags
	[d appendData:[tag dataUsingEncoding: NSUTF8StringEncoding]]; // item tag
	char c = 0; [d appendBytes:&c length:1]; // 0x00 separator after tag
	[d appendData:data]; // item value
	return d;
}

-(bool) isString { //returns whether data is UTF-8 string(or implicitly can be converted to)
	return (flags & TAG_FIELD_FLAG_DATA_TYPE_MASK) == TAG_FIELD_FLAG_DATA_TYPE_TEXT_UTF8;
}
-(NSString*) getString { //returns string, if is not string - this is error, check isString before
	if ([self isString]) return [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
	return nil;
}
-(NSString*) tag { return tag; }
@end


@implementation ApeTag
+(id)create { return [[ApeTag alloc] init];}
+(id)createFromFileRead:(NSFileHandle*)f { return [[ApeTag alloc] initFromFileRead:f]; }

-(id)init {
	self = [super init];
	if (self)
	{
		fields = [NSMutableArray array];
	}
	return self;
}

-(id)initFromFileRead:(NSFileHandle*)f {
	self = [super init];
	if(self)
	{
		fields = [NSMutableArray array];
		if (![self read:f])
			return nil;
	}
	return self;
}

-(void) addField:(NSString*)f text:(NSString*)t {
	[fields addObject:[ApeTagItem createTag:f flags:(TAG_FIELD_FLAG_DATA_TYPE_TEXT_UTF8) data:[t dataUsingEncoding: NSUTF8StringEncoding]]];
}

-(NSArray*) fields { return fields; }

-(NSDictionary*) convertToCogTag {
	//NSLog(@"Converting ape tag to cog tag");
	NSMutableDictionary* d = [NSMutableDictionary dictionaryWithCapacity:6];
	NSEnumerator *e = [fields objectEnumerator]; 
	ApeTagItem* item; 
	int n = 0;
	while ((item = [e nextObject]) != nil) {
		if (![[item tag] compare:APE_TAG_FIELD_ARTIST]) { [d setObject:[item getString] forKey:@"artist"]; 	n++;}
		if (![[item tag] compare:APE_TAG_FIELD_ALBUM])	{[d setObject:[item getString] forKey:@"album"];	n++;}
		if (![[item tag] compare:APE_TAG_FIELD_TITLE])	{[d setObject:[item getString] forKey:@"title"];	n++;}
		if (![[item tag] compare:APE_TAG_FIELD_TRACK])	{[d setObject:[NSNumber numberWithInt:[[item getString] intValue]] forKey:@"track"];	n++;}
		if (![[item tag] compare:APE_TAG_FIELD_GENRE])	{[d setObject:[item getString] forKey:@"genre"];	n++;}
		if (![[item tag] compare:APE_TAG_FIELD_YEAR])	{[d setObject:[item getString] forKey:@"year"];		n++;}
	}
	if (n)
		return d;
	[d release]; d = nil;
	return nil;
}

-(NSData*) pack {
	int len = 0, num = 0;
	NSMutableData* d = [NSMutableData dataWithCapacity:8];
	NSEnumerator *e = [fields objectEnumerator]; 
	id item; 
	while ((item = [e nextObject]) != nil) 
	{
		NSData* i = [item pack];
		[d appendData:i];
		len += [i length];
		num++;
	}
	UInt32 version = CFSwapInt32HostToLittle(CURRENT_APE_TAG_VERSION); 
	UInt32 size = CFSwapInt32HostToLittle(len + APE_TAG_FOOTER_BYTES);
	UInt32 count = CFSwapInt32HostToLittle(num);
	UInt32 flags = CFSwapInt32HostToLittle(APE_TAG_FLAGS_DEFAULT);
	[d appendBytes:"APETAGEX" length:8];
	[d appendBytes:&version length:4];
	[d appendBytes:&size length:4];
	[d appendBytes:&count length:4];
	[d appendBytes:&flags length:4];
	[d appendBytes:"\0\0\0\0\0\0\0\0" length:8]; //reserved
	return d;	
}

-(void) write:(NSFileHandle*)file {
	[file writeData:[self pack]];
}

-(NSString*) readToNull:(NSFileHandle*)f { //!!!
	NSMutableData* d = [NSMutableData dataWithCapacity:100]; //it will grow, here should be most expected value (may gain few nanosecond from it =) )
	while(true) {
		NSData* byte = [f readDataOfLength:1];
		if (!byte)
		{
			[f seekToFileOffset:0-[d length]];
			return nil;
		}
		if (*((const char*)[byte bytes]) == 0) break;
		[d appendData:byte];
	}
	NSString* s = [[NSString alloc] initWithData:d encoding:NSUTF8StringEncoding]; //!handle encoding error (in case binary data starts not from newline, impossible on real apl, but who knows...)
	return s;
}

-(bool) read:(NSFileHandle*)f { //reads from file, returns true on success
	
	//reading from file erases previous data:
	[fields removeAllObjects];
	
	//!! prefix header may be present and we should read it but lazyness...
	NSString* header = @"APETAGEX";
	NSData* check = [f readDataOfLength:[header length]];
	[f seekToFileOffset:([f offsetInFile]-[check length])];
	if (check && [[[NSString alloc] initWithData:check encoding:NSASCIIStringEncoding] isEqualToString:header]) {
		NSLog(@"Reading of prefix header not implemented");
		return false;
	}
	
	//read fields and store 'em
	while(true) {
		NSData* check = [f readDataOfLength:[header length]];
		[f seekToFileOffset:([f offsetInFile]-[check length])];
		if (check && [[[NSString alloc] initWithData:check encoding:NSASCIIStringEncoding] isEqualToString:header]) break; // reached footer
		
		NSData* f_len = [f readDataOfLength:4];
		NSData* f_flag = [f readDataOfLength:4];
		NSString* f_name = [self readToNull:f];
		UInt32 len = CFSwapInt32LittleToHost(*(uint32_t*)[f_len bytes]);
		UInt32 flags = CFSwapInt32LittleToHost(*(uint32_t*)[f_flag bytes]);
		NSData* data = [f readDataOfLength:len];
		ApeTagItem* item = [ApeTagItem createTag:f_name flags:flags data:data];
		//NSLog(@"Read tag '%@'='%@'", [item tag], [item getString]);
		[fields addObject:item];
	}
	//here we should read footer and check number of fields etc. - but who cares? =)
	// just clean up file pointer:
	[f readDataOfLength:APE_TAG_FOOTER_BYTES];
	return true;
}

@end
