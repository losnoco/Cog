#import <Cocoa/Cocoa.h>

//not full support!! and even may have not enougth checks to find a non-supported data!



@interface ApeTagItem: NSObject {
	UInt32 flags;
	NSString* tag;
	NSData* data;
}

+createTag:(NSString*)t flags:(SInt32)f data:(NSData*)d;
-initTag:(NSString*)t flags:(SInt32)f data:(NSData*)d;

//for writing:
-(NSData*) pack;

//for reading:
-(NSString*) tag;
-(bool) isString; //returns whether data is UTF-8 string(or implicitly can be converted to)
-(NSString*) getString; //returns string, if is not string - this is error, check isString before
@end




@interface ApeTag : NSObject {
	NSMutableArray* fields;
}

+(id)create;
+(id)createFromFileRead:(NSFileHandle*)f;
-(id)init;
-(id)initFromFileRead:(NSFileHandle*)f;

-(NSArray*) fields; // returns fields array

-(void) addField:(NSString*)f text:(NSString*)t;
-(NSData*) pack; //returns comoplete tag, ready for writing to file/transmitting smwhere

-(void) write:(NSFileHandle*)f; //writes to file output of -pack
-(bool) read:(NSFileHandle*)f; //reads from file, returns true on success

-(NSDictionary*) convertToCogTag;
@end
