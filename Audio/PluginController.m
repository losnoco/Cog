#import "PluginController.h"
#import "CogPluginMulti.h"
#import "Plugin.h"

#import "Logging.h"

#import "NSFileHandle+CreateFile.h"

@implementation PluginController

@synthesize sources;
@synthesize containers;
@synthesize metadataReaders;

@synthesize propertiesReadersByExtension;
@synthesize propertiesReadersByMimeType;

@synthesize decodersByExtension;
@synthesize decodersByMimeType;

@synthesize configured;

static PluginController *sharedPluginController = nil;

+ (id<CogPluginController>)sharedPluginController {
	@synchronized(self) {
		if(sharedPluginController == nil) {
			sharedPluginController = [[self alloc] init];
		}
	}

	return sharedPluginController;
}

- (id)init {
	self = [super init];
	if(self) {
		self.sources = [[NSMutableDictionary alloc] init];
		self.containers = [[NSMutableDictionary alloc] init];

		self.metadataReaders = [[NSMutableDictionary alloc] init];

		self.propertiesReadersByExtension = [[NSMutableDictionary alloc] init];
		self.propertiesReadersByMimeType = [[NSMutableDictionary alloc] init];

		self.decodersByExtension = [[NSMutableDictionary alloc] init];
		self.decodersByMimeType = [[NSMutableDictionary alloc] init];

		[self setup];
	}

	return self;
}

- (void)setup {
	if(self.configured == NO) {
		self.configured = YES;

		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(bundleDidLoad:) name:NSBundleDidLoadNotification object:nil];

		[self loadPlugins];
		[self printPluginInfo];
	}
}

- (void)bundleDidLoad:(NSNotification *)notification {
	NSArray *classNames = [[notification userInfo] objectForKey:@"NSLoadedClasses"];
	for(NSString *className in classNames) {
		Class bundleClass = NSClassFromString(className);
		if([bundleClass conformsToProtocol:@protocol(CogVersionCheck)]) {
			DLog(@"Component has version check: %@", className);
			if(![bundleClass shouldLoadForOSVersion:[[NSProcessInfo processInfo] operatingSystemVersion]]) {
				DLog(@"Plugin fails OS version check, ignoring");
				return;
			}
		}
	}
	for(NSString *className in classNames) {
		DLog(@"Class loaded: %@", className);
		Class bundleClass = NSClassFromString(className);
		if([bundleClass conformsToProtocol:@protocol(CogContainer)]) {
			[self setupContainer:className];
		}
		if([bundleClass conformsToProtocol:@protocol(CogDecoder)]) {
			[self setupDecoder:className];
		}
		if([bundleClass conformsToProtocol:@protocol(CogMetadataReader)]) {
			[self setupMetadataReader:className];
		}
		if([bundleClass conformsToProtocol:@protocol(CogPropertiesReader)]) {
			[self setupPropertiesReader:className];
		}
		if([bundleClass conformsToProtocol:@protocol(CogSource)]) {
			[self setupSource:className];
		}
	}
}

- (void)loadPluginsAtPath:(NSString *)path {
	NSArray *dirContents = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:path error:nil];

	for(NSString *pname in dirContents) {
		NSString *ppath;
		ppath = [NSString pathWithComponents:@[path, pname]];

		if([[pname pathExtension] isEqualToString:@"bundle"]) {
			NSBundle *b = [NSBundle bundleWithPath:ppath];
			[b load];
		}
	}
}

- (void)loadPlugins {
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
	NSString *basePath = [[paths firstObject] stringByAppendingPathComponent:@"Cog"];

	[self loadPluginsAtPath:[[NSBundle mainBundle] builtInPlugInsPath]];
	[self loadPluginsAtPath:[basePath stringByAppendingPathComponent:@"Plugins"]];
}

- (void)setupContainer:(NSString *)className {
	Class container = NSClassFromString(className);
	if(container && [container respondsToSelector:@selector(fileTypes)]) {
		for(id fileType in [container fileTypes]) {
			NSString *ext = [fileType lowercaseString];
			NSMutableArray *containerSet;
			if(![containers objectForKey:ext]) {
				containerSet = [[NSMutableArray alloc] init];
				[containers setObject:containerSet forKey:ext];
			} else
				containerSet = [containers objectForKey:ext];
			[containerSet addObject:className];
		}
	}
}

- (void)setupDecoder:(NSString *)className {
	Class decoder = NSClassFromString(className);
	if(decoder && [decoder respondsToSelector:@selector(fileTypes)]) {
		for(id fileType in [decoder fileTypes]) {
			NSString *ext = [fileType lowercaseString];
			NSMutableArray *decoders;
			if(![decodersByExtension objectForKey:ext]) {
				decoders = [[NSMutableArray alloc] init];
				[decodersByExtension setObject:decoders forKey:ext];
			} else
				decoders = [decodersByExtension objectForKey:ext];
			[decoders addObject:className];
		}
	}

	if(decoder && [decoder respondsToSelector:@selector(mimeTypes)]) {
		for(id mimeType in [decoder mimeTypes]) {
			NSString *mimetype = [mimeType lowercaseString];
			NSMutableArray *decoders;
			if(![decodersByMimeType objectForKey:mimetype]) {
				decoders = [[NSMutableArray alloc] init];
				[decodersByMimeType setObject:decoders forKey:mimetype];
			} else
				decoders = [decodersByMimeType objectForKey:mimetype];
			[decoders addObject:className];
		}
	}
}

- (void)setupMetadataReader:(NSString *)className {
	Class metadataReader = NSClassFromString(className);
	if(metadataReader && [metadataReader respondsToSelector:@selector(fileTypes)]) {
		for(id fileType in [metadataReader fileTypes]) {
			NSString *ext = [fileType lowercaseString];
			NSMutableArray *readers;
			if(![metadataReaders objectForKey:ext]) {
				readers = [[NSMutableArray alloc] init];
				[metadataReaders setObject:readers forKey:ext];
			} else
				readers = [metadataReaders objectForKey:ext];
			[readers addObject:className];
		}
	}
}

- (void)setupPropertiesReader:(NSString *)className {
	Class propertiesReader = NSClassFromString(className);
	if(propertiesReader && [propertiesReader respondsToSelector:@selector(fileTypes)]) {
		for(id fileType in [propertiesReader fileTypes]) {
			NSString *ext = [fileType lowercaseString];
			NSMutableArray *readers;
			if(![propertiesReadersByExtension objectForKey:ext]) {
				readers = [[NSMutableArray alloc] init];
				[propertiesReadersByExtension setObject:readers forKey:ext];
			} else
				readers = [propertiesReadersByExtension objectForKey:ext];
			[readers addObject:className];
		}
	}

	if(propertiesReader && [propertiesReader respondsToSelector:@selector(mimeTypes)]) {
		for(id mimeType in [propertiesReader mimeTypes]) {
			NSString *mimetype = [mimeType lowercaseString];
			NSMutableArray *readers;
			if(![propertiesReadersByMimeType objectForKey:mimetype]) {
				readers = [[NSMutableArray alloc] init];
				[propertiesReadersByMimeType setObject:readers forKey:mimetype];
			} else
				readers = [propertiesReadersByMimeType objectForKey:mimetype];
			[readers addObject:className];
		}
	}
}

- (void)setupSource:(NSString *)className {
	Class source = NSClassFromString(className);
	if(source && [source respondsToSelector:@selector(schemes)]) {
		for(id scheme in [source schemes]) {
			[sources setObject:className forKey:scheme];
		}
	}
}

- (void)printPluginInfo {
	ALog(@"Sources: %@", self.sources);
	ALog(@"Containers: %@", self.containers);
	ALog(@"Metadata Readers: %@", self.metadataReaders);

	ALog(@"Properties Readers By Extension: %@", self.propertiesReadersByExtension);
	ALog(@"Properties Readers By Mime Type: %@", self.propertiesReadersByMimeType);

	ALog(@"Decoders by Extension: %@", self.decodersByExtension);
	ALog(@"Decoders by Mime Type: %@", self.decodersByMimeType);

#if 0
    // XXX Keep in sync with Info.plist on disk!
    NSString * plistHeader = @"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n\
<plist version=\"1.0\">\n\
<dict>\n\
\t<key>CFBundleDevelopmentRegion</key>\n\
\t<string>English</string>\n\
\t<key>CFBundleDocumentTypes</key>\n\
\t<array>\n\
\t\t<dict>\n\
\t\t\t<key>CFBundleTypeExtensions</key>\n\
\t\t\t<array>\n\
\t\t\t\t<string>*</string>\n\
\t\t\t</array>\n\
\t\t\t<key>CFBundleTypeIconFile</key>\n\
\t\t\t<string>song.icns</string>\n\
\t\t\t<key>CFBundleTypeOSTypes</key>\n\
\t\t\t<array>\n\
\t\t\t\t<string>****</string>\n\
\t\t\t\t<string>fold</string>\n\
\t\t\t\t<string>disk</string>\n\
\t\t\t</array>\n\
\t\t\t<key>CFBundleTypeRole</key>\n\
\t\t\t<string>None</string>\n\
\t\t</dict>\n";
    NSString * plistFooter = @"\t</array>\n\
\t<key>CFBundleExecutable</key>\n\
\t<string>Cog</string>\n\
\t<key>CFBundleHelpBookFolder</key>\n\
\t<string>Cog.help</string>\n\
\t<key>CFBundleHelpBookName</key>\n\
\t<string>org.cogx.cog.help</string>\n\
\t<key>CFBundleIdentifier</key>\n\
\t<string>$(PRODUCT_BUNDLE_IDENTIFIER)</string>\n\
\t<key>CFBundleInfoDictionaryVersion</key>\n\
\t<string>6.0</string>\n\
\t<key>CFBundlePackageType</key>\n\
\t<string>APPL</string>\n\
\t<key>CFBundleShortVersionString</key>\n\
\t<string>0.08</string>\n\
\t<key>CFBundleSignature</key>\n\
\t<string>????</string>\n\
\t<key>CFBundleVersion</key>\n\
\t<string>r516</string>\n\
\t<key>LSApplicationCategoryType</key>\n\
\t<string>public.app-category.music</string>\n\
\t<key>LSMinimumSystemVersion</key>\n\
\t<string>$(MACOSX_DEPLOYMENT_TARGET)</string>\n\
\t<key>NSAppTransportSecurity</key>\n\
\t<dict>\n\
\t\t<key>NSAllowsArbitraryLoads</key>\n\
\t\t<true/>\n\
\t</dict>\n\
\t<key>NSAppleScriptEnabled</key>\n\
\t<string>YES</string>\n\
\t<key>NSCalendarsUsageDescription</key>\n\
\t<string>Cog has no use for your calendar information. Why are you trying to open your Calendar with an audio player?</string>\n\
\t<key>NSCameraUsageDescription</key>\n\
\t<string>Cog is an audio player. It will never use your camera. Why is it asking for permission to use your camera?</string>\n\
\t<key>NSContactsUsageDescription</key>\n\
\t<string>Cog has no use for your contacts information. Why are you trying to open your contacts with an audio player?</string>\n\
\t<key>NSLocationUsageDescription</key>\n\
\t<string>Cog has no use for your location information. Something is obviously wrong with the application.</string>\n\
\t<key>NSMainNibFile</key>\n\
\t<string>MainMenu</string>\n\
\t<key>NSMicrophoneUsageDescription</key>\n\
\t<string>Cog is an audio player. It does not, however, record audio. Why is it asking for permission to use your microphone?</string>\n\
\t<key>NSPhotoLibraryUsageDescription</key>\n\
\t<string>Cog is an audio player. Why are you trying to access your Photos Library with an audio player?</string>\n\
\t<key>NSPrincipalClass</key>\n\
\t<string>MediaKeysApplication</string>\n\
\t<key>NSRemindersUsageDescription</key>\n\
\t<string>Cog has no use for your reminders. Why are you trying to access them with an audio player?</string>\n\
\t<key>SUFeedURL</key>\n\
\t<string>https://cogcdn.cog.losno.co/mercury.xml</string>\n\
\t<key>SUPublicEDKey</key>\n\
\t<string>omxG7Rp0XK9/YEvKbVy7cd44eVAh1LJB6CmjQwjOJz4=</string>\n\
</dict>\n\
</plist>\n";
    NSMutableArray * decodersRegistered = [[NSMutableArray alloc] init];
    
    NSArray * allKeys = [self.decodersByExtension allKeys];
    for (NSString * ext in allKeys) {
        NSArray * decoders = [self.decodersByExtension objectForKey:ext];
        for (NSString * decoder in decoders) {
            if (![decodersRegistered containsObject:decoder]) {
                [decodersRegistered addObject:decoder];
            }
        }
    }

    NSMutableArray * stringList = [[NSMutableArray alloc] init];

    [stringList addObject:plistHeader];

    // These aren't handled by decoders, but as containers
    NSArray * staticTypes = @[
        @[@"M3U Playlist File", @"m3u.icns", @"m3u", @"m3u8"],
        @[@"PLS Playlist File", @"pls.icns", @"pls"],
        @[@"RAR Archive of SPC Files", @"vg.icns", @"rsn"],
        @[@"7Z Archive of VGM Files", @"vg.icns", @"vgm7z"]
    ];

    NSMutableArray * assocTypes = [[NSMutableArray alloc] init];

    [assocTypes addObjectsFromArray:staticTypes];
    
    for (NSString * decoderString in decodersRegistered) {
        Class decoder = NSClassFromString(decoderString);
        if (decoder && [decoder respondsToSelector:@selector(fileTypeAssociations)]) {
            NSArray * types = [decoder fileTypeAssociations];
            [assocTypes addObjectsFromArray:types];
        }
    }

    for (NSArray * type in assocTypes) {
        [stringList addObject:@"\t\t<dict>\n\
\t\t\t<key>CFBundleTypeExtensions</key>\n\
\t\t\t<array>\n\
"];
        for (size_t i = 2; i < [type count]; ++i) {
            [stringList addObject:@"\t\t\t\t<string>"];
            [stringList addObject:[[type objectAtIndex:i] lowercaseString]];
            [stringList addObject:@"</string>\n"];
        }
        [stringList addObject:@"\t\t\t</array>\n\
\t\t\t<key>CFBundleTypeIconFile</key>\n\
\t\t\t<string>"];
        [stringList addObject:[type objectAtIndex:1]];
        [stringList addObject:@"</string>\n\
\t\t\t<key>CFBundleTypeName</key>\n\
\t\t\t<string>"];
        [stringList addObject:[type objectAtIndex:0]];
        [stringList addObject:@"</string>\n\
\t\t\t<key>CFBundleTypeRole</key>\n\
\t\t\t<string>Viewer</string>\n\
\t\t\t<key>LSTypeIsPackage</key>\n\
\t\t\t<false/>\n\
\t\t</dict>\n"];
    }
    
    [stringList addObject:plistFooter];
    
    NSFileHandle *fileHandle = [NSFileHandle fileHandleForWritingAtPath:@"/tmp/Cog_Info.plist" createFile:YES];
    if (!fileHandle) {
        DLog(@"Error saving Info.plist!");
        return;
    }
    [fileHandle truncateFileAtOffset:0];
    [fileHandle writeData:[[stringList componentsJoinedByString:@""] dataUsingEncoding:NSUTF8StringEncoding]];
    [fileHandle closeFile];
#endif
}

- (id<CogSource>)audioSourceForURL:(NSURL *)url {
	NSString *scheme = [url scheme];

	Class source = NSClassFromString([sources objectForKey:scheme]);

	return [[source alloc] init];
}

- (NSArray *)urlsForContainerURL:(NSURL *)url {
	NSString *ext = [url pathExtension];
	NSArray *containerSet = [containers objectForKey:[ext lowercaseString]];
	NSString *classString;
	if(containerSet) {
		if([containerSet count] > 1) {
			return [CogContainerMulti urlsForContainerURL:url containers:containerSet];
		} else {
			classString = [containerSet objectAtIndex:0];
		}
	} else {
		return nil;
	}

	Class container = NSClassFromString(classString);

	return [container urlsForContainerURL:url];
}

// Note: Source is assumed to already be opened.
- (id<CogDecoder>)audioDecoderForSource:(id<CogSource>)source skipCue:(BOOL)skip {
	NSString *ext = [[source url] pathExtension];
	NSArray *decoders = [decodersByExtension objectForKey:[ext lowercaseString]];
	NSString *classString;
	if(decoders) {
		if([decoders count] > 1) {
			if(skip) {
				NSMutableArray *_decoders = [decoders mutableCopy];
				for(int i = 0; i < [_decoders count];) {
					if([[_decoders objectAtIndex:i] isEqualToString:@"CueSheetDecoder"])
						[_decoders removeObjectAtIndex:i];
					else
						++i;
				}
				return [[CogDecoderMulti alloc] initWithDecoders:_decoders];
			}
			return [[CogDecoderMulti alloc] initWithDecoders:decoders];
		} else {
			classString = [decoders objectAtIndex:0];
		}
	} else {
		decoders = [decodersByMimeType objectForKey:[[source mimeType] lowercaseString]];
		if(decoders) {
			if([decoders count] > 1) {
				return [[CogDecoderMulti alloc] initWithDecoders:decoders];
			} else {
				classString = [decoders objectAtIndex:0];
			}
		} else {
			classString = @"SilenceDecoder";
		}
	}

	Class decoder = NSClassFromString(classString);

	return [[decoder alloc] init];
}

- (NSDictionary *)metadataForURL:(NSURL *)url skipCue:(BOOL)skip {
	NSString *urlScheme = [url scheme];
	if([urlScheme isEqualToString:@"http"] ||
	   [urlScheme isEqualToString:@"https"])
		return nil;

	NSString *ext = [url pathExtension];
	NSArray *readers = [metadataReaders objectForKey:[ext lowercaseString]];
	NSString *classString;
	if(readers) {
		if([readers count] > 1) {
			if(skip) {
				NSMutableArray *_readers = [readers mutableCopy];
				for(int i = 0; i < [_readers count];) {
					if([[_readers objectAtIndex:i] isEqualToString:@"CueSheetMetadataReader"])
						[_readers removeObjectAtIndex:i];
					else
						++i;
				}
				return [CogMetadataReaderMulti metadataForURL:url readers:_readers];
			}
			return [CogMetadataReaderMulti metadataForURL:url readers:readers];
		} else {
			classString = [readers objectAtIndex:0];
		}
	} else {
		return nil;
	}

	Class metadataReader = NSClassFromString(classString);

	return [metadataReader metadataForURL:url];
}

// If no properties reader is defined, use the decoder's properties.
- (NSDictionary *)propertiesForURL:(NSURL *)url {
	NSString *urlScheme = [url scheme];
	if([urlScheme isEqualToString:@"http"] ||
	   [urlScheme isEqualToString:@"https"])
		return nil;

	NSDictionary *properties = nil;
	NSString *ext = [url pathExtension];

	id<CogSource> source = [self audioSourceForURL:url];
	if(![source open:url])
		return nil;

	NSArray *readers = [propertiesReadersByExtension objectForKey:[ext lowercaseString]];
	NSString *classString = nil;
	if(readers) {
		if([readers count] > 1) {
			properties = [CogPropertiesReaderMulti propertiesForSource:source readers:readers];
			if(properties != nil && [properties count])
				return properties;
		} else {
			classString = [readers objectAtIndex:0];
		}
	} else {
		readers = [propertiesReadersByMimeType objectForKey:[[source mimeType] lowercaseString]];
		if(readers) {
			if([readers count] > 1) {
				properties = [CogPropertiesReaderMulti propertiesForSource:source readers:readers];
				if(properties != nil && [properties count])
					return properties;
			} else {
				classString = [readers objectAtIndex:0];
			}
		}
	}

	if(classString) {
		Class propertiesReader = NSClassFromString(classString);

		properties = [propertiesReader propertiesForSource:source];
		if(properties != nil && [properties count])
			return properties;
	}

	{
		id<CogDecoder> decoder = [self audioDecoderForSource:source skipCue:NO];
		if(![decoder open:source]) {
			return nil;
		}

		NSDictionary *properties = [decoder properties];

		[decoder close];

		return properties;
	}
}

- (int)putMetadataInURL:(NSURL *)url {
	return 0;
}

@end
