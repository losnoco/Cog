/*
	Are defines really appropriate for this?
	We want something easily insertable into a dictionary.
	Maybe should extern these, and shove the instances in PluginController.m, but will that cause linking problems?
*/

#define	kCogSource 				@"CogSource"
#define kCogContainer 			@"CogContainer"
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
- (NSString *)mimeType;

- (BOOL)open:(NSURL *)url;
- (BOOL)seekable;
- (BOOL)seek:(long)position whence:(int)whence;
- (long)tell;
- (int)read:(void *)buffer amount:(int)amount; //reads UP TO amount, returns amount read.
- (void)close;
@end

@protocol CogContainer <NSObject> 
+ (NSArray *)fileTypes; //mp3, ogg, etc
+ (NSArray *)mimeTypes;

+ (NSArray *)urlsForContainerURL:(NSURL *)url;
@end

@protocol CogDecoder <NSObject> 
+ (NSArray *)mimeTypes;
+ (NSArray *)fileTypes; //mp3, ogg, etc

//For KVO
//- (void)setProperties:(NSDictionary *)p;
- (NSDictionary *)properties;

- (BOOL)open:(id<CogSource>)source;
- (double)seekToTime:(double)time; //time is in milleseconds, should return the time actually seeked to.
- (int)fillBuffer:(void *)buf ofSize:(UInt32)size;
- (void)close;
@end

@protocol CogMetadataReader <NSObject>
+ (NSArray *)fileTypes;
+ (NSArray *)mimeTypes;
+ (NSDictionary *)metadataForURL:(NSURL *)url;
@end

@protocol CogPropertiesReader <NSObject>
+ (NSArray *)fileTypes;
+ (NSArray *)mimeTypes;
+ (NSDictionary *)propertiesForSource:(id<CogSource>)source;
@end

@protocol CogPluginController <NSObject>
- (id<CogSource>) audioSourceForURL:(NSURL *)url;
- (NSArray *) urlsForContainerURL:(NSURL *)url;
- (NSDictionary *) metadataForURL:(NSURL *)url;
- (NSDictionary *) propertiesForURL:(NSURL *)url;

- (id<CogDecoder>) audioDecoderForSource:(id<CogSource>)source;
@end



