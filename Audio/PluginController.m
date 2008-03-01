#import "PluginController.h"
#import "Plugin.h"

@implementation PluginController

//Start of singleton-related stuff.
static PluginController *sharedPluginController = nil;

+ (id<CogPluginController>)sharedPluginController
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
		containers = [[NSMutableDictionary alloc] init];

		metadataReaders = [[NSMutableDictionary alloc] init];

		propertiesReadersByExtension = [[NSMutableDictionary alloc] init];
		propertiesReadersByMimeType = [[NSMutableDictionary alloc] init];

		decodersByExtension = [[NSMutableDictionary alloc] init];
		decodersByMimeType = [[NSMutableDictionary alloc] init];

		[self setup];
	}
	
	return self;
}

- (void)setup
{
	if (isSetup == NO) {
		isSetup = YES;
		
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(bundleDidLoad:) name:NSBundleDidLoadNotification object:nil];

		[self loadPlugins];
		[self printPluginInfo];
	}	
}

- (void)bundleDidLoad:(NSNotification *)notification
{
	NSArray *classNames = [[notification userInfo] objectForKey:@"NSLoadedClasses"];
	for (NSString *className in classNames)
	{
		NSLog(@"Class loaded: %@", className);
		Class bundleClass = NSClassFromString(className);
		if ([bundleClass conformsToProtocol:@protocol(CogContainer)]) {
			[self setupContainer:className];
		}
		else if ([bundleClass conformsToProtocol:@protocol(CogDecoder)]) {
			[self setupDecoder:className];
		}
		else if ([bundleClass conformsToProtocol:@protocol(CogMetadataReader)]) {
			[self setupMetadataReader:className];
		}
		else if ([bundleClass conformsToProtocol:@protocol(CogPropertiesReader)]) {
			[self setupPropertiesReader:className];
		}
		else if ([bundleClass conformsToProtocol:@protocol(CogSource)]) {
			[self setupSource:className];
		}
		else {
			NSLog(@"Unknown plugin type!!");
		}
	}
}

- (void)loadPluginsAtPath:(NSString *)path
{

	NSArray *dirContents = [[NSFileManager defaultManager] directoryContentsAtPath:path];
	NSEnumerator *dirEnum = [dirContents objectEnumerator];

	NSString *pname;

	while (pname = [dirEnum nextObject])
	{
		NSString *ppath;
		ppath = [NSString pathWithComponents:[NSArray arrayWithObjects:path,pname,nil]];
		
		if ([[pname pathExtension] isEqualToString:@"bundle"])
		{
			NSBundle *b = [NSBundle bundleWithPath:ppath];
			[b load];
		}
	}
}

- (void)loadPlugins
{
	[self loadPluginsAtPath:[[NSBundle mainBundle] builtInPlugInsPath]];
	[self loadPluginsAtPath:[@"~/Library/Application Support/Cog/Plugins" stringByExpandingTildeInPath]];
}

- (void)setupContainer:(NSString *)className
{
	Class container = NSClassFromString(className);
	if (container && [container respondsToSelector:@selector(fileTypes)]) {
		NSEnumerator *fileTypesEnum = [[container fileTypes] objectEnumerator];
		id fileType;
		while (fileType = [fileTypesEnum nextObject])
		{
			[containers setObject:className forKey:[fileType lowercaseString]];
		}
	}
}

- (void)setupDecoder:(NSString *)className
{
	Class decoder = NSClassFromString(className);
	if (decoder && [decoder respondsToSelector:@selector(fileTypes)]) {
		id fileType;
		NSEnumerator *fileTypesEnum = [[decoder fileTypes] objectEnumerator];
		while (fileType = [fileTypesEnum nextObject])
		{
			[decodersByExtension setObject:className forKey:[fileType lowercaseString]];
		}
	}
	
	if (decoder && [decoder respondsToSelector:@selector(mimeTypes)]) {
		id mimeType;
		NSEnumerator *mimeTypesEnum = [[decoder mimeTypes] objectEnumerator];
		while (mimeType = [mimeTypesEnum nextObject])
		{
			[decodersByMimeType setObject:className forKey:[mimeType lowercaseString]];
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
			[propertiesReadersByExtension setObject:className forKey:[fileType lowercaseString]];
		}
	}

	if (propertiesReader && [propertiesReader respondsToSelector:@selector(mimeTypes)]) {
		id mimeType;
		NSEnumerator *mimeTypesEnum = [[propertiesReader mimeTypes] objectEnumerator];
		while (mimeType = [mimeTypesEnum nextObject])
		{
			[propertiesReadersByMimeType setObject:className forKey:[mimeType lowercaseString]];
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
	NSLog(@"Containers: %@", containers);
	NSLog(@"Metadata Readers: %@", metadataReaders);

	NSLog(@"Properties Readers By Extension: %@", propertiesReadersByExtension);
	NSLog(@"Properties Readers By Mime Type: %@", propertiesReadersByMimeType);

	NSLog(@"Decoders by Extension: %@", decodersByExtension);
	NSLog(@"Decoders by Mime Type: %@", decodersByMimeType);
}

- (NSDictionary *)sources
{
	return sources;
}

- (NSDictionary *)containers
{
	return containers;
}

- (NSDictionary *)decodersByExtension
{
	return decodersByExtension;
}

- (NSDictionary *)decodersByMimeType
{
	return decodersByMimeType;
}

- (NSDictionary *)propertiesReadersByExtension
{
	return propertiesReadersByExtension;
}

- (NSDictionary *)propertiesReadersByMimeType
{
	return propertiesReadersByMimeType;
}

- (NSDictionary *)metadataReaders
{
	return metadataReaders;
}


- (id<CogSource>) audioSourceForURL:(NSURL *)url
{
	NSString *scheme = [url scheme];
	
	Class source = NSClassFromString([sources objectForKey:scheme]);
	
	return [[[source alloc] init] autorelease];
}

- (NSArray *) urlsForContainerURL:(NSURL *)url
{
	NSString *ext = [[url path] pathExtension];
	
	Class container = NSClassFromString([containers objectForKey:[ext lowercaseString]]);
	
	return [container urlsForContainerURL:url];
}

//Note: Source is assumed to already be opened.
- (id<CogDecoder>) audioDecoderForSource:(id <CogSource>)source
{
	NSString *ext = [[[source url] path] pathExtension];
	NSString *classString = [decodersByExtension objectForKey:[ext lowercaseString]];
	if (!classString) {
		classString = [decodersByMimeType objectForKey:[[source mimeType] lowercaseString]];
	}

	Class decoder = NSClassFromString(classString);
	
	return [[[decoder alloc] init] autorelease];
}

- (NSDictionary *)metadataForURL:(NSURL *)url
{
	NSString *ext = [[url path] pathExtension];
	
	Class metadataReader = NSClassFromString([metadataReaders objectForKey:[ext lowercaseString]]);
	
	return [metadataReader metadataForURL:url];
	
}


//If no properties reader is defined, use the decoder's properties.
- (NSDictionary *)propertiesForURL:(NSURL *)url
{
	NSString *ext = [[url path] pathExtension];
	
	id<CogSource> source = [self audioSourceForURL:url];
	if (![source open:url])
		return nil;

	NSString *classString = [propertiesReadersByExtension objectForKey:[ext lowercaseString]];
	if (!classString) {
		classString = [propertiesReadersByMimeType objectForKey:[[source mimeType] lowercaseString]];
	}

	if (classString)
	{
		Class propertiesReader = NSClassFromString(classString);

		 return [propertiesReader propertiesForSource:source];
	}
	else
	{
	
		id<CogDecoder> decoder = [self audioDecoderForSource:source];
		if (![decoder open:source])
		{
			return nil;
		}
		
		NSDictionary *properties = [decoder properties];
		
		[decoder close];
		
		return properties;
	}
}

@end

