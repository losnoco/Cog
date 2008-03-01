#import "PluginController.h"
#import "Plugin.h"

@implementation PluginController

@synthesize sources;
@synthesize containers;
@synthesize metadataReaders;

@synthesize propertiesReadersByExtension;
@synthesize propertiesReadersByMimeType;

@synthesize decodersByExtension;
@synthesize decodersByMimeType;

@synthesize configured;

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
	if (self.configured == NO) {
		self.configured == YES;
		
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

	for (NSString *pname in dirContents)
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
		for (id fileType in [container fileTypes])
		{
			[containers setObject:className forKey:[fileType lowercaseString]];
		}
	}
}

- (void)setupDecoder:(NSString *)className
{
	Class decoder = NSClassFromString(className);
	if (decoder && [decoder respondsToSelector:@selector(fileTypes)]) {
		for (id fileType in [decoder fileTypes])
		{
			[decodersByExtension setObject:className forKey:[fileType lowercaseString]];
		}
	}
	
	if (decoder && [decoder respondsToSelector:@selector(mimeTypes)]) {
		for (id mimeType in [decoder mimeTypes]) 
		{
			[decodersByMimeType setObject:className forKey:[mimeType lowercaseString]];
		}
	}
}

- (void)setupMetadataReader:(NSString *)className
{
	Class metadataReader = NSClassFromString(className);
	if (metadataReader && [metadataReader respondsToSelector:@selector(fileTypes)]) {
		for (id fileType in [metadataReader fileTypes])
		{
			[metadataReaders setObject:className forKey:[fileType lowercaseString]];
		}
	}
}

- (void)setupPropertiesReader:(NSString *)className
{
	Class propertiesReader = NSClassFromString(className);
	if (propertiesReader && [propertiesReader respondsToSelector:@selector(fileTypes)]) {
		for (id fileType in [propertiesReader fileTypes])
		{
			[propertiesReadersByExtension setObject:className forKey:[fileType lowercaseString]];
		}
	}

	if (propertiesReader && [propertiesReader respondsToSelector:@selector(mimeTypes)]) {
		for (id mimeType in [propertiesReader mimeTypes])
		{
			[propertiesReadersByMimeType setObject:className forKey:[mimeType lowercaseString]];
		}
	}
}

- (void)setupSource:(NSString *)className
{
	Class source = NSClassFromString(className);
	if (source && [source respondsToSelector:@selector(schemes)]) {
		for (id scheme in [source schemes])
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

