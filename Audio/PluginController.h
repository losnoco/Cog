/* PluginController */

#import <Cocoa/Cocoa.h>

//Singleton
@interface PluginController : NSObject
{
	NSMutableArray *codecPlugins;

	NSMutableDictionary *decoders;
	NSMutableDictionary *metadataReaders;
	NSMutableDictionary *propertiesReaders;
}

+ (PluginController *)sharedPluginController; //Use this to get the instance.

- (void)setup;

- (void)loadPlugins;
- (void)setupPlugins;

- (void)printPluginInfo;

- (NSDictionary *)decoders;
- (NSDictionary *)metadataReaders;
- (NSDictionary *)propertiesReaders;

@end
