
#import <Cocoa/Cocoa.h>

@interface APLFile : NSObject {
	long startBlock;
	long endBlock;
	NSURL* file;
}
+createWithFile:(NSString*)f;
-initWithFile:(NSString*)f;

-(long)startBlock; // always return 0 or greater!
-(long)endBlock;
-(NSURL*) file;

	//writing support to be added in far future
@end
