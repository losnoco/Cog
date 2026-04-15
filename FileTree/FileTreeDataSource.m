//
//  FileTreeDataSource.m
//  Cog
//
//  Created by Vincent Spader on 10/14/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "FileTreeDataSource.h"

#import "DirectoryNode.h"
#import "FileNode.h"
#import "PathWatcher.h"

#import "Logging.h"

#import "AppController.h"

#import "SandboxBroker.h"

#import "CogAudio/AudioPlayer.h"

static void *kFileTreeDataSourceContext = &kFileTreeDataSourceContext;

// XXX this is only for reference, we have the entitlement for the path anyway
static NSURL *pathEscape(NSString *path) {
	NSString *componentsToRemove = [NSString stringWithFormat:@"Library/Containers/%@/Data/", [[NSBundle mainBundle] bundleIdentifier]];
	NSRange rangeOfMatch = [path rangeOfString:componentsToRemove];
	if(rangeOfMatch.location != NSNotFound)
		path = [path stringByReplacingCharactersInRange:rangeOfMatch withString:@""];
	return [NSURL fileURLWithPath:path];
}

static NSURL *defaultMusicDirectory(void) {
	NSString *path = [NSSearchPathForDirectoriesInDomains(NSMusicDirectory, NSUserDomainMask, YES) lastObject];
	return pathEscape(path);
}

@interface FileTreeDataSource ()

@property NSURL *rootURL;

@property const void *sbHandle;

@end

@implementation FileTreeDataSource {
	PathNode *rootNode;
	const void *_sbHandle;
	NSArray *filteredNodes;
	NSString *_filterString;
	id _keyEventMonitor;
}

@synthesize filterString = _filterString;

+ (void)initialize {
	NSString *path = [defaultMusicDirectory() absoluteString];
	NSDictionary *userDefaultsValuesDict = @{ @"fileTreeRootURL": path };
	[[NSUserDefaults standardUserDefaults] registerDefaults:userDefaultsValuesDict];
}

- (void)awakeFromNib {
	_sbHandle = NULL;
	[self.pathControl setTarget:self];
	[self.pathControl setAction:@selector(pathControlAction:)];
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self
	                                                          forKeyPath:@"values.fileTreeRootURL"
	                                                             options:NSKeyValueObservingOptionNew |
	                                                                     NSKeyValueObservingOptionInitial
	                                                             context:kFileTreeDataSourceContext];

	// Build search field programmatically so AppKit sets up all internal
	// geometry (icon inset, field-editor frame, cancel button hit area) correctly.
	NSScrollView *scrollView = [self.outlineView enclosingScrollView];
	NSView *container = [scrollView superview];
	NSRect scrollFrame = [scrollView frame];
	const CGFloat searchHeight = 22.0;
	const CGFloat newScrollHeight = scrollFrame.size.height - searchHeight;
	[scrollView setFrame:NSMakeRect(scrollFrame.origin.x, scrollFrame.origin.y,
	                                scrollFrame.size.width, newScrollHeight)];
	NSRect searchFrame = NSMakeRect(scrollFrame.origin.x,
	                                scrollFrame.origin.y + newScrollHeight,
	                                scrollFrame.size.width, searchHeight);
	NSSearchField *sf = [[NSSearchField alloc] initWithFrame:searchFrame];
	[sf setPlaceholderString:NSLocalizedString(@"Search", @"File tree search placeholder")];
	[sf setDelegate:self];
	[sf setAutoresizingMask:NSViewWidthSizable | NSViewMinYMargin];
	[container addSubview:sf];
	self.searchField = sf;

	__weak typeof(self) weakSelf = self;
	_keyEventMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyDown
	                                                        handler:^NSEvent *(NSEvent *event) {
		NSEventModifierFlags flags = event.modifierFlags &
		    (NSEventModifierFlagCommand | NSEventModifierFlagOption |
		     NSEventModifierFlagShift | NSEventModifierFlagControl);
		if(flags == (NSEventModifierFlagCommand | NSEventModifierFlagOption) &&
		   [[event charactersIgnoringModifiers] isEqualToString:@"f"]) {
			FileTreeDataSource *s = weakSelf;
			if(s && s.searchField && [s.searchField.window isKeyWindow]) {
				[s.searchField.window makeFirstResponder:s.searchField];
				return nil;
			}
		}
		return event;
	}];
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context {
	if(context == kFileTreeDataSourceContext) {
		if([keyPath isEqualToString:@"values.fileTreeRootURL"]) {
			NSString *url =
			[[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"fileTreeRootURL"];
			DLog(@"File tree root URL: %@\n", url);
			self.rootURL = [NSURL URLWithString:url];
		}
	} else {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
	}
}

- (void)changeURL:(NSURL *)url {
	if(url != nil) {
		[[[NSUserDefaultsController sharedUserDefaultsController] defaults] setObject:[url absoluteString]
		                                                                       forKey:@"fileTreeRootURL"];
	}
}

- (void)pathControlAction:(id)sender {
	NSPathControlItem *item = [self.pathControl clickedPathItem];
	if(item != nil && item.URL != nil) {
		[self changeURL:item.URL];
	}
}

- (NSURL *)rootURL {
	return [rootNode URL];
}

- (void)setFilterString:(NSString *)filterString {
	_filterString = [filterString copy];
	if(!filterString.length) {
		filteredNodes = nil;
		[self.outlineView reloadData];
	} else {
		[self performSearchWithString:filterString];
	}
}

- (void)controlTextDidChange:(NSNotification *)notification {
	self.filterString = [(NSSearchField *)[notification object] stringValue];
}

- (void)searchFieldDidEndSearching:(NSSearchField *)sender {
	self.filterString = @"";
}

- (void)performSearchWithString:(NSString *)searchString {
	NSString *captured = [searchString copy];
	NSURL *rootURL = self.rootURL;
	if(!rootURL) return;

	dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
		NSMutableArray *results = [NSMutableArray array];
		NSString *rootPath = [rootURL path];
		NSString *rootPathWithSlash = [rootPath stringByAppendingString:@"/"];

		NSDirectoryEnumerator<NSURL *> *enumerator =
		    [[NSFileManager defaultManager] enumeratorAtURL:rootURL
		                        includingPropertiesForKeys:@[NSURLNameKey, NSURLIsDirectoryKey]
		                                           options:NSDirectoryEnumerationSkipsHiddenFiles
		                                      errorHandler:nil];

		for(NSURL *url in enumerator) {
			if(![captured isEqualToString:self->_filterString]) return;

			NSString *name = [url lastPathComponent];
			if([name rangeOfString:captured options:NSCaseInsensitiveSearch].location == NSNotFound)
				continue;

			NSNumber *isDirNum = nil;
			[url getResourceValue:&isDirNum forKey:NSURLIsDirectoryKey error:nil];
			BOOL isDir = [isDirNum boolValue];

			if(!isDir && ![[AudioPlayer fileTypes] containsObject:[[url pathExtension] lowercaseString]])
				continue;

			NSString *fullPath = [url path];
			NSString *relativePath = [fullPath hasPrefix:rootPathWithSlash]
			    ? [fullPath substringFromIndex:rootPathWithSlash.length]
			    : fullPath;

			PathNode *node = isDir
			    ? [[DirectoryNode alloc] initWithDataSource:self url:url]
			    : [[FileNode alloc] initWithDataSource:self url:url];
			[node setDisplay:relativePath];
			[results addObject:node];

			if(results.count >= 500) break;
		}

		dispatch_async(dispatch_get_main_queue(), ^{
			if([captured isEqualToString:self->_filterString]) {
				self->filteredNodes = [results copy];
				[self.outlineView reloadData];
			}
		});
	});
}

- (void)setRootURL:(NSURL *)rootURL {
	SandboxBroker *sharedSandboxBroker = [SandboxBroker sharedSandboxBroker];
	if(self.sbHandle) [sharedSandboxBroker endFolderAccess:self.sbHandle];
	self.sbHandle = [sharedSandboxBroker beginFolderAccess:rootURL];

	if(![[NSFileManager defaultManager] fileExistsAtPath:[rootURL path]]) {
		rootURL = defaultMusicDirectory();
	}

	rootNode = [[DirectoryNode alloc] initWithDataSource:self url:rootURL];

	[self.watcher setPath:[rootURL path]];

	[self reloadPathNode:rootNode];
}

- (const void *)sbHandle {
	return _sbHandle;
}

- (void)setSbHandle:(const void *)sbHandle {
	_sbHandle = sbHandle;
}

- (void)dealloc {
	if(_keyEventMonitor) [NSEvent removeMonitor:_keyEventMonitor];
	if(self.sbHandle) [[SandboxBroker sharedSandboxBroker] endFolderAccess:self.sbHandle];
}

- (PathNode *)nodeForPath:(NSString *)path {
	NSString *relativePath = [[path stringByReplacingOccurrencesOfString:[[[self rootURL] path] stringByAppendingString:@"/"]
	                                                          withString:@""
	                                                             options:NSAnchoredSearch
	                                                               range:NSMakeRange(0, [path length])] stringByStandardizingPath];
	if([relativePath isEqualToString:[[self rootURL] path]])
		relativePath = @"";
	PathNode *node = rootNode;
	DLog(@"Root | Relative | Path: %@ | %@ | %@", [[self rootURL] path], relativePath, path);
	for(NSString *c in [relativePath pathComponents]) {
		DLog(@"COMPONENT: %@", c);
		PathNode *subnode = [[node subpathsLookup] objectForKey:c];
		if(!subnode) return nil;
		node = subnode;
	}

	return node;
}

- (void)pathDidChange:(NSString *)path flags:(FSEventStreamEventFlags)flags {
	if(!(flags & (kFSEventStreamEventFlagItemCreated |
	              kFSEventStreamEventFlagItemRemoved |
	              kFSEventStreamEventFlagItemRenamed))) {
		// We only care about changes that would affect whether a file exists, or has a new name
		// Apparently, Xattr changes are causing crashes in this notification tracker, oops.
		return;
	}
	DLog(@"PATH DID CHANGE: %@", path);
	// Need to find the corresponding node...and call [node reloadPath], then [self reloadPathNode:node]
	PathNode *node;
	do {
		node = [self nodeForPath:path];
		path = [path stringByDeletingLastPathComponent];
		if(!path || [path length] < 2) return;
	} while(!node);

	if(flags & kFSEventStreamEventFlagItemRemoved) {
		DLog(@"Removing node: %@", node);
		PathNode *parentNode = [self nodeForPath:path];
		[parentNode updatePath];
		[self reloadPathNode:parentNode];
	} else {
		DLog(@"NODE IS: %@", node);
		[node updatePath];
		[self reloadPathNode:node];
	}
}

- (NSInteger)outlineView:(NSOutlineView *)outlineView numberOfChildrenOfItem:(id)item {
	if(filteredNodes)
		return item == nil ? (NSInteger)filteredNodes.count : 0;
	PathNode *n = (item == nil ? rootNode : item);
	return (int)[[n subpaths] count];
}

- (BOOL)outlineView:(NSOutlineView *)outlineView isItemExpandable:(id)item {
	if(filteredNodes) return NO;
	PathNode *n = (item == nil ? rootNode : item);
	return ![n isLeaf];
}

- (id)outlineView:(NSOutlineView *)outlineView child:(NSInteger)index ofItem:(id)item {
	if(filteredNodes)
		return filteredNodes[(NSUInteger)index];
	PathNode *n = (item == nil ? rootNode : item);
	return [n subpaths][(NSUInteger)index];
}

- (id)outlineView:(NSOutlineView *)outlineView objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item {
	PathNode *n = (item == nil ? rootNode : item);
	return n;
}

- (id<NSPasteboardWriting>)outlineView:(NSOutlineView *)outlineView pasteboardWriterForItem:(id)item {
	NSPasteboardItem *paste = [NSPasteboardItem new];
	[paste setData:[[item URL] dataRepresentation] forType:NSPasteboardTypeFileURL];
	return paste;
}

- (void)reloadPathNode:(PathNode *)item {
	if(filteredNodes) return; // filesystem changes don't update the search results view
	if(item == rootNode) {
		[self.outlineView reloadData];
	} else {
		[self.outlineView reloadItem:item reloadChildren:YES];
	}
}

@end
