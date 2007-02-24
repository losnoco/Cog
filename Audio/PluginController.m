#import "PluginController.h"
#import "Plugin.h"

@implementation PluginController

//Start of singleton-related stuff.
static PluginController *sharedPluginController = nil;

+ (PluginController*)sharedPluginController
{
	@synchronized(self) {
		if (sharedPluginController == nil) {
			[[self alloc] init]; // assignment not done here
		}
	}
	return sharedPluginController;
}

+ (id)allocWithZone:(NSZone *)zone
{
	@synchronized(self) {
		if (sharedPluginController == nil) {
			sharedPluginController = [super allocWithZone:zone];
			return sharedPluginController;  // assignment and return on first allocation
		}
	}

	return nil; //on subsequent allocation attempts return nil
}

- (id)copyWithZone:(NSZone *)zone
{
	return self;
}

- (id)retain
{
	return self;
}

- (unsigned)retainCount
{
	return UINT_MAX;  //denotes an object that cannot be released
}
 
- (void)release
{
	//do nothing
}

- (id)autorelease
{
	return self;
}

//End of singleton-related stuff

- (id)init {
	self = [super init];
	if (self) {
		codecPlugins = [[NSMutableArray alloc] init];

		decoders = [[NSMutableDictionary alloc] init];
		metadataReaders = [[NSMutableDictionary alloc] init];
		propertiesReaders = [[NSMutableDictionary alloc] init];
	}
	
	return self;
}

- (void)setup
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	[self loadPlugins];
	[self setupPlugins];
	[self printPluginInfo];

	[pool release];
}

- (void)loadPluginsAtPath:(NSString *)path
{

	NSArray *dirContents = [[NSFileManager defaultManager] directoryContentsAtPath:path];
	NSEnumerator *dirEnum = [dirContents objectEnumerator];

	NSString *pname;

	NSLog(@"Loading plugins at %@, candidates: %@", path, dirContents);
	while (pname = [dirEnum nextObject])
	{
		NSString *ppath;
		ppath = [NSString pathWithComponents:[NSArray arrayWithObjects:path,pname,nil]];
		
		if ([[pname pathExtension] isEqualToString:@"bundle"])
		{
			NSBundle *b = [NSBundle bundleWithPath:ppath];
			if (b)
			{
				NSLog(@"Loaded bundle: %@", b);
				id plugin = [[[b principalClass] alloc] init];
				NSLog(@"Candidate: %@", plugin);
				if ([plugin respondsToSelector:@selector(pluginType)])
				{
					NSLog(@"Responds to selector");
					switch([plugin pluginType])
					{
						case kCogPluginCodec: //Only type currently
							NSLog(@"Its a codec");
							[codecPlugins addObject:plugin];
							break;
						default:
							NSLog(@"Unknown plugin type");
							break;
					}
				}
			}
		}
	}
}

- (void)loadPlugins
{
	[self loadPluginsAtPath:[[NSBundle mainBundle] builtInPlugInsPath]];
	[self loadPluginsAtPath:[@"~/Library/Application Support/Cog/Plugins" stringByExpandingTildeInPath]];
}

- (void)setupInputPlugins
{
	NSEnumerator *pluginsEnum = [codecPlugins objectEnumerator];
	id plugin;
	while (plugin = [pluginsEnum nextObject])
	{
		Class decoder = [plugin decoder];
		Class metadataReader = [plugin metadataReader];
		Class propertiesReader = [plugin propertiesReader];

		if (decoder) {
			NSEnumerator *fileTypesEnum = [[decoder fileTypes] objectEnumerator];
			id fileType;
			NSString *classString = NSStringFromClass(decoder);
			while (fileType = [fileTypesEnum nextObject])
			{
				[decoders setObject:classString forKey:fileType];
			}
		}

		if (metadataReader) {
			NSEnumerator *fileTypesEnum = [[metadataReader fileTypes] objectEnumerator];
			id fileType;
			NSString *classString = NSStringFromClass(metadataReader);
			while (fileType = [fileTypesEnum nextObject])
			{
				[metadataReaders setObject:classString forKey:fileType];
			}
		}

		if (propertiesReader) {
			NSEnumerator *fileTypesEnum = [[propertiesReader fileTypes] objectEnumerator];
			id fileType;
			NSString *classString = NSStringFromClass(propertiesReader);
			while (fileType = [fileTypesEnum nextObject])
			{
				[propertiesReaders setObject:classString forKey:fileType];
			}
		}
	}
}

- (void)setupPlugins {
	[self setupInputPlugins];
}

- (void)printPluginInfo
{
	NSLog(@"Codecs: %@\n\n", codecPlugins);
}

- (NSDictionary *)decoders
{
	return decoders;
}

- (NSDictionary *)metadataReaders
{
	return metadataReaders;
}

- (NSDictionary *)propertiesReaders
{
	return propertiesReaders;
}

@end

//This is called when the framework is loaded.
void __attribute__ ((constructor)) InitializePlugins(void) {
	static BOOL wasInitialized = NO; 
	if (!wasInitialized) {
		// safety in case we get called twice.
		[[PluginController sharedPluginController] setup];

		wasInitialized = YES;
	}
}
