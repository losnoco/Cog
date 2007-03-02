/* PluginController */

#import <Cocoa/Cocoa.h>

//Singleton
@interface PluginController : NSObject
{
	NSMutableDictionary *sources;
	NSMutableDictionary *decoders;
	NSMutableDictionary *metadataReaders;
	NSMutableDictionary *propertiesReaders;
}

+ (PluginController *)sharedPluginController; //Use this to get the instance.

- (void)setup;
- (void)printPluginInfo;

- (void)loadPlugins; 
- (void)loadPluginsAtPath:(NSString *)path;

- (void)setupSource:(NSString *)className;
- (void)setupDecoder:(NSString *)className;
- (void)setupMetadataReader:(NSString *)className;
- (void)setupPropertiesReader:(NSString *)className;

- (NSDictionary *)sources;
- (NSDictionary *)decoders;
- (NSDictionary *)metadataReaders;
- (NSDictionary *)propertiesReaders;

@end
