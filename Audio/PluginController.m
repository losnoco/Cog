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
		sources = [[NSMutableDictionary alloc] init];
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
				Class plugin = [b principalClass];
				NSLog(@"Candidate: %@", plugin);
				if ([plugin respondsToSelector:@selector(pluginInfo)])
				{
					NSLog(@"Responds to selector...");
					//PluginInfo is a dictionary that contains keys/values like pluginClass,classType...ex: VorbisDecoder, Decoder
					NSDictionary *pluginInfo = [plugin pluginInfo];
					NSEnumerator *e = [pluginInfo keyEnumerator];
					id className;
					while (className = [e nextObject]) {
						id pluginType = [pluginInfo objectForKey:className];
						if ([pluginType isEqualToString:kCogDecoder]) {
							NSLog(@"DECODER");
							[self setupDecoder:className];
						}
						else if ([pluginType isEqualToString:kCogMetadataReader]) {
							NSLog(@"Metadata");
							[self setupMetadataReader:className];
						}
						else if ([pluginType isEqualToString:kCogPropertiesReader]) {
							NSLog(@"Properties");
							[self setupPropertiesReader:className];
						}
						else if ([pluginType isEqualToString:kCogSource]) {
							NSLog(@"Source");
							[self setupSource:className];
						}
						else {
							NSLog(@"Unknown plugin type!!");
						}
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

- (void)setupDecoder:(NSString *)className
{
	Class decoder = NSClassFromString(className);
	if (decoder && [decoder respondsToSelector:@selector(fileTypes)]) {
		NSEnumerator *fileTypesEnum = [[decoder fileTypes] objectEnumerator];
		id fileType;
		while (fileType = [fileTypesEnum nextObject])
		{
			[decoders setObject:className forKey:[fileType lowercaseString]];
		}
	}
}

- (void)setupMetadataReader:(NSString *)className
{
	Class metadataReader = NSClassFromString(className);
	if (metadataReader && [metadataReader respondsToSelector:@selector(fileTypes)]) {
		NSEnumerator *fileTypesEnum = [[metadataReader fileTypes] objectEnumerator];
		id fileType;
		while (fileType = [fileTypesEnum nextObject])
		{
			[metadataReaders setObject:className forKey:[fileType lowercaseString]];
		}
	}
}

- (void)setupPropertiesReader:(NSString *)className
{
	Class propertiesReader = NSClassFromString(className);
	if (propertiesReader && [propertiesReader respondsToSelector:@selector(fileTypes)]) {
		NSEnumerator *fileTypesEnum = [[propertiesReader fileTypes] objectEnumerator];
		id fileType;
		while (fileType = [fileTypesEnum nextObject])
		{
			[propertiesReaders setObject:className forKey:[fileType lowercaseString]];
		}
	}
}

- (void)setupSource:(NSString *)className
{
	Class source = NSClassFromString(className);
	if (source && [source respondsToSelector:@selector(schemes)]) {
		NSEnumerator *schemeEnum = [[source schemes] objectEnumerator];
		id scheme;
		while (scheme = [schemeEnum nextObject])
		{
			[sources setObject:className forKey:scheme];
		}
	}
}

- (void)printPluginInfo
{
	NSLog(@"Sources: %@", sources);
	NSLog(@"Decoders: %@", decoders);
	NSLog(@"Metadata Readers: %@", metadataReaders);
	NSLog(@"Properties Readers: %@", propertiesReaders);
}

- (NSDictionary *)sources
{
	return sources;
}

- (NSDictionary *)decoders
{
	return decoders;
}

- (NSDictionary *)propertiesReaders
{
	return propertiesReaders;
}

- (NSDictionary *)metadataReaders
{
	return metadataReaders;
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
