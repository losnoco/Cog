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
	
	BOOL configured;
}

@property(retain) NSMutableDictionary *sources;
@property(retain) NSMutableDictionary *containers;
@property(retain) NSMutableDictionary *metadataReaders;

@property(retain) NSMutableDictionary *propertiesReadersByExtension;
@property(retain) NSMutableDictionary *propertiesReadersByMimeType;

@property(retain) NSMutableDictionary *decodersByExtension;
@property(retain) NSMutableDictionary *decodersByMimeType;

@property BOOL configured;

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
