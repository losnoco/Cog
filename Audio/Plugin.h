/*
	Are defines really appropriate for this?
	We want something easily insertable into a dictionary.
	Maybe should extern these, and shove the instances in PluginController.m, but will that cause linking problems?
*/

#define	kCogSource 				@"CogSource"
#define kCogDecoder 			@"CogDecoder"
#define kCogMetadataReader 		@"CogMetadataReader"
#define kCogPropertiesReader 	@"CogPropertiesReader"

@protocol CogPlugin <NSObject>
//Dictionary containing classname/plugintype pairs. ex: @"VorbisDecoder": kCogDecoder, @"VorbisPropertiesRaeder": kCogPropertiesReader
+ (NSDictionary *)pluginInfo;
@end

@protocol CogSource <NSObject>
+ (NSArray *)schemes; //http, file, etc

- (NSURL *)url;

- (BOOL)open:(NSURL *)url;
- (NSDictionary *)properties; //Perhaps contains header info for HTTP stream, or path for a regular file.
- (BOOL)seekable;
- (BOOL)seek:(long)position whence:(int)whence;
- (long)tell;
- (int)read:(void *)buffer amount:(int)amount; //reads UP TO amount, returns amount read.
- (void)close;
@end

@protocol CogDecoder <NSObject> 
+ (NSArray *)fileTypes; //mp3, ogg, etc

//For KVO
//- (void)setProperties:(NSDictionary *)p;
- (NSDictionary *)properties;

- (BOOL)open:(id<CogSource>)source;
- (BOOL)seekable;
- (double)seekToTime:(double)time; //time is in milleseconds, should return the time actually seeked to.
- (int)fillBuffer:(void *)buf ofSize:(UInt32)size;
- (void)close;
@end

@protocol CogMetadataReader <NSObject>
+ (NSArray *)fileTypes;
+ (NSDictionary *)metadataForURL;
@end

@protocol CogPropertiesReader <NSObject>
+ (NSArray *)fileTypes;
+ (NSDictionary *)propertiesForSource:(id<CogSource>)source;
@end



