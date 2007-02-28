typedef enum 
{
	kCogPluginCodec = 1,
	kCogPluginGeneral
} PluginType;

@protocol CogPlugin
- (PluginType)pluginType;
@end

@protocol CogCodecPlugin
- (Class)decoder;
- (Class)metadataReader;
- (Class)propertiesReader;
@end

@protocol CogDecoder
+ (NSArray *)fileTypes;

- (BOOL)open:(NSURL *)url;
- (NSDictionary *)properties;
- (double)seekToTime:(double)time;
- (int)fillBuffer:(void *)buf ofSize:(UInt32)size;
- (void)close;
@end

@protocol CogMetadataReader
+ (NSArray *)fileTypes;
@end

@protocol CogPropertiesReader
+ (NSArray *)fileTypes;
@end


