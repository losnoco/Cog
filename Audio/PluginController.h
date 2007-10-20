/* PluginController */

#import <Cocoa/Cocoa.h>

#import "Plugin.h"

//Singleton
@interface PluginController : NSObject <CogPluginController>
{
	NSMutableDictionary *sources;
	NSMutableDictionary *containers;
	NSMutableDictionary *metadataReaders;

	NSMutableDictionary *propertiesReadersByExtension;
	NSMutableDictionary *propertiesReadersByMimeType;

	NSMutableDictionary *decodersByExtension;
	NSMutableDictionary *decodersByMimeType;
	
	BOOL isSetup;
}

+ (PluginController *)sharedPluginController; //Use this to get the instance.

- (void)setup;
- (void)printPluginInfo;

- (void)loadPlugins; 
- (void)loadPluginsAtPath:(NSString *)path;

- (void)setupSource:(NSString *)className;
- (void)setupContainer:(NSString *)className;
- (void)setupDecoder:(NSString *)className;
- (void)setupMetadataReader:(NSString *)className;
- (void)setupPropertiesReader:(NSString *)className;

@end
