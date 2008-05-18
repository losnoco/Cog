
#import <Cocoa/Cocoa.h>

@class ApeTag;

@interface APLFile : NSObject {
	long startBlock;
	long endBlock;
	ApeTag* tag;
	NSURL* file;
}
+createWithFile:(NSString*)f;
-initWithFile:(NSString*)f;

-(long)startBlock; // always return 0 or greater!
-(long)endBlock;
-(ApeTag*) tag;
-(NSURL*) file;

	//writing support to be added in far future
@end
