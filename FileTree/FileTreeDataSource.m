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

static NSURL *defaultMusicDirectory(void) {
    return [[NSFileManager defaultManager] URLForDirectory:NSMusicDirectory
                                                  inDomain:NSUserDomainMask
                                         appropriateForURL:nil
                                                    create:NO
                                                     error:nil];
}

@interface FileTreeDataSource()

@property NSURL *rootURL;

@end

@implementation FileTreeDataSource {
    PathNode *rootNode;
}

+ (void)initialize {
    NSString *path = [defaultMusicDirectory() absoluteString];
    NSDictionary *userDefaultsValuesDict = @{@"fileTreeRootURL": path};
    [[NSUserDefaults standardUserDefaults] registerDefaults:userDefaultsValuesDict];
}

- (void)awakeFromNib {
    [self.pathControl setTarget:self];
    [self.pathControl setAction:@selector(pathControlAction:)];
    [[NSUserDefaultsController sharedUserDefaultsController] addObserver:self
                                                              forKeyPath:@"values.fileTreeRootURL"
                                                                 options:NSKeyValueObservingOptionNew |
                                                                         NSKeyValueObservingOptionInitial
                                                                 context:nil];
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context {
    if ([keyPath isEqualToString:@"values.fileTreeRootURL"]) {
        NSString *url =
                [[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"fileTreeRootURL"];
        DLog(@"File tree root URL: %@\n", url);
        self.rootURL = [NSURL URLWithString:url];
    }
}

- (void)changeURL:(NSURL *)url {
    if (url != nil) {
        [[[NSUserDefaultsController sharedUserDefaultsController] defaults] setObject:[url absoluteString]
                                                                               forKey:@"fileTreeRootURL"];
    }
}

- (void)pathControlAction:(id)sender {
    NSPathControlItem *item = [self.pathControl clickedPathItem];
    if (item != nil && item.URL != nil) {
        [self changeURL:item.URL];
    }
}

- (NSURL *)rootURL {
    return [rootNode URL];
}

- (void)setRootURL:(NSURL *)rootURL {
    if (![[NSFileManager defaultManager] fileExistsAtPath:[rootURL path]]) {
        rootURL = defaultMusicDirectory();
    }

    rootNode = [[DirectoryNode alloc] initWithDataSource:self url:rootURL];

    [self.watcher setPath:[rootURL path]];

    [self reloadPathNode:rootNode];
}

- (PathNode *)nodeForPath:(NSString *)path {
    NSString *relativePath = [[path stringByReplacingOccurrencesOfString:[[[self rootURL] path] stringByAppendingString:@"/"]
                                                              withString:@""
                                                                 options:NSAnchoredSearch
                                                                   range:NSMakeRange(0, [path length])
    ] stringByStandardizingPath];
    PathNode *node = rootNode;
    DLog(@"Root | Relative | Path: %@ | %@ | %@", [[self rootURL] path], relativePath, path);
    for (NSString *c in [relativePath pathComponents]) {
        DLog(@"COMPONENT: %@", c);
        BOOL found = NO;
        for (PathNode *subnode in [node subpaths]) {
            if ([[[[subnode URL] path] lastPathComponent] isEqualToString:c]) {
                node = subnode;
                found = YES;
            }
        }

        if (!found) {
            DLog(@"Not found!");
            return nil;
        }
    }

    return node;
}

- (void)pathDidChange:(NSString *)path {
    DLog(@"PATH DID CHANGE: %@", path);
    //Need to find the corresponding node...and call [node reloadPath], then [self reloadPathNode:node]
    PathNode *node = [self nodeForPath:path];
    DLog(@"NODE IS: %@", node);
    [node updatePath];
    [self reloadPathNode:node];
}

- (NSInteger)outlineView:(NSOutlineView *)outlineView numberOfChildrenOfItem:(id)item {
    PathNode *n = (item == nil ? rootNode : item);

    return (int) [[n subpaths] count];
}

- (BOOL)outlineView:(NSOutlineView *)outlineView isItemExpandable:(id)item {
    PathNode *n = (item == nil ? rootNode : item);

    return ![n isLeaf];
}

- (id)outlineView:(NSOutlineView *)outlineView child:(NSInteger)index ofItem:(id)item {
    PathNode *n = (item == nil ? rootNode : item);

    return [n subpaths][(NSUInteger) index];
}

- (id)outlineView:(NSOutlineView *)outlineView objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item {
    PathNode *n = (item == nil ? rootNode : item);

    return n;
}

- (id <NSPasteboardWriting>)outlineView:(NSOutlineView *)outlineView pasteboardWriterForItem:(id)item {
    NSPasteboardItem *paste = [[NSPasteboardItem alloc] init];
    [paste setData:[[item URL] dataRepresentation] forType:NSPasteboardTypeFileURL];
    return paste;
}

- (void)reloadPathNode:(PathNode *)item {
    if (item == rootNode) {
        [self.outlineView reloadData];
    } else {
        [self.outlineView reloadItem:item reloadChildren:YES];
    }
}

@end
