//
//  FileTreeDataSource.m
//  Cog
//
//  Created by Vincent Spader on 10/14/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "FileTreeDataSource.h"

#import "DirectoryNode.h"
#import "PathWatcher.h"

#import "Logging.h"

#import "AppController.h"

#import "SandboxBroker.h"

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
}

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
	PathNode *n = (item == nil ? rootNode : item);

	return (int)[[n subpaths] count];
}

- (BOOL)outlineView:(NSOutlineView *)outlineView isItemExpandable:(id)item {
	PathNode *n = (item == nil ? rootNode : item);

	return ![n isLeaf];
}

- (id)outlineView:(NSOutlineView *)outlineView child:(NSInteger)index ofItem:(id)item {
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
	if(item == rootNode) {
		[self.outlineView reloadData];
	} else {
		[self.outlineView reloadItem:item reloadChildren:YES];
	}
}

@end
