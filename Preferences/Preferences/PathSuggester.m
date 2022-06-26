//
//  PathSuggester.m
//  Preferences
//
//  Created by Christopher Snowhill on 6/21/22.
//

#import "PathSuggester.h"

#import "PlaylistController.h"

#import "SandboxBroker.h"

#import "AudioContainer.h"

// Sync, only declare items we need
@interface PlaylistEntry
@property(nonatomic) NSURL *_Nullable url;
@end

static NSURL *containerDirectory(void) {
	NSString *path = [@"~" stringByExpandingTildeInPath];
	return [NSURL fileURLWithPath:path];
}

// XXX this is only for comparison, not "escaping the sandbox"
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

static NSURL *defaultDownloadsDirectory(void) {
	NSString *path = [NSSearchPathForDirectoriesInDomains(NSDownloadsDirectory, NSUserDomainMask, YES) lastObject];
	return pathEscape(path);
}

static NSURL *defaultMoviesDirectory(void) {
	NSString *path = [NSSearchPathForDirectoriesInDomains(NSMoviesDirectory, NSUserDomainMask, YES) lastObject];
	return pathEscape(path);
}

@interface PathItem : NSObject
@property(nonatomic, strong) NSString *path;
@property(nonatomic) BOOL enabled;

- (IBAction)clickEnable:(id)sender;
@end

@implementation PathItem
@synthesize path;
@synthesize enabled;

- (IBAction)clickEnable:(id)sender {
	self.enabled = !self.enabled;
}
@end

@interface PathSuggester ()

@end

@implementation PathSuggester

- (id)init {
	return [super initWithWindowNibName:@"PathSuggester"];
}

- (IBAction)beginSuggestion:(id)sender {
	[self showWindow:self];

	[pathsList removeObjects:[pathsList arrangedObjects]];

	NSPersistentContainer *pc = [NSClassFromString(@"PlaylistController") sharedPersistentContainer];

	NSPredicate *hasUrlPredicate = [NSPredicate predicateWithFormat:@"urlString != nil && urlString != %@", @""];
	NSPredicate *deletedPredicate = [NSPredicate predicateWithFormat:@"deLeted == NO || deLeted == nil"];

	NSCompoundPredicate *predicate = [NSCompoundPredicate andPredicateWithSubpredicates:@[deletedPredicate, hasUrlPredicate]];

	NSFetchRequest *request = [NSFetchRequest fetchRequestWithEntityName:@"PlaylistEntry"];
	request.predicate = predicate;

	NSError *error = nil;
	NSArray *results = [pc.viewContext executeFetchRequest:request error:&error];

	if(!results || [results count] < 1) return;

	NSURL *defaultMusic = defaultMusicDirectory();
	NSURL *defaultDownloads = defaultDownloadsDirectory();
	NSURL *defaultMovies = defaultMoviesDirectory();

	NSURL *container = containerDirectory();

	id sandboxBrokerClass = NSClassFromString(@"SandboxBroker");

	NSMutableArray *items = [[NSMutableArray alloc] init];
	NSMutableArray *itemPaths = [[NSMutableArray alloc] init];

	NSArray *originalArray = [results valueForKey:@"url"];
	NSMutableArray *array = [originalArray mutableCopy];

	for(NSURL *url in originalArray) {
		NSArray *containedUrls = [AudioContainer dependencyUrlsForContainerURL:url];
		if(containedUrls && [containedUrls count] > 0) {
			[array addObjectsFromArray:containedUrls];
		}
	}

	// Add other system paths to this setting
	NSString *fileTreePath = [[NSUserDefaults standardUserDefaults] stringForKey:@"fileTreeRootURL"];
	if(fileTreePath && [fileTreePath length]) {
		[array addObject:[NSURL URLWithString:fileTreePath]];
	}

	NSString *soundFontPath = [[NSUserDefaults standardUserDefaults] stringForKey:@"soundFontPath"];
	if(soundFontPath && [soundFontPath length]) {
		[array addObject:[NSURL fileURLWithPath:soundFontPath]];
	}

	for(NSURL *fileUrl in array) {
		NSURL *url = [sandboxBrokerClass urlWithoutFragment:fileUrl];
		if([sandboxBrokerClass isPath:url aSubdirectoryOf:defaultMusic] ||
		   [sandboxBrokerClass isPath:url
		              aSubdirectoryOf:defaultDownloads] ||
		   [sandboxBrokerClass isPath:url
		              aSubdirectoryOf:defaultMovies] ||
		   [sandboxBrokerClass isPath:url
		              aSubdirectoryOf:container] ||
		   [sandboxPathBehaviorController matchesPath:url])
			continue;

		NSArray *pathComponents = [url pathComponents];
		for(size_t i = 2; i < [pathComponents count]; ++i) {
			NSURL *subUrl = [NSURL fileURLWithPathComponents:[pathComponents subarrayWithRange:NSMakeRange(0, i)]];
			if(![itemPaths containsObject:subUrl]) {
				[itemPaths addObject:subUrl];
				PathItem *item = [[PathItem alloc] init];
				item.path = [subUrl path];
				item.enabled = NO;
				[items addObject:item];
			}
		}
	}

	NSSortDescriptor *sortDescriptor = [NSSortDescriptor sortDescriptorWithKey:@"path.length" ascending:YES];
	[pathsList setSortDescriptors:@[sortDescriptor]];

	[items sortUsingDescriptors:@[sortDescriptor]];

	[pathsList addObjects:items];

	[pathsList setSelectedObjects:@[]];
}

- (void)windowDidLoad {
	[super windowDidLoad];
}

- (NSView *_Nullable)tableView:(NSTableView *)tableView viewForTableColumn:(NSTableColumn *_Nullable)tableColumn row:(NSInteger)row {
	NSString *cellIdentifier = @"";
	NSTextAlignment cellTextAlignment = NSTextAlignmentLeft;

	PathItem *pi = [[pathsList arrangedObjects] objectAtIndex:row];

	float fontSize = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] floatForKey:@"fontSize"];

	NSButton *cellButton = nil;
	NSString *cellText = @"";

	if(pi) {
		cellIdentifier = [tableColumn identifier];
		if([cellIdentifier isEqualToString:@"enabled"]) {
			cellButton = [NSButton checkboxWithTitle:@"" target:pi action:@selector(clickEnable:)];
			cellButton.state = pi.enabled ? NSControlStateValueOn : NSControlStateValueOff;
		} else if([cellIdentifier isEqualToString:@"path"]) {
			cellText = pi.path;
		}
	}

	NSView *view = [tableView makeViewWithIdentifier:cellIdentifier owner:nil];
	if(view) {
		NSTableCellView *cellView = (NSTableCellView *)view;

		if(cellView.textField) {
			cellView.textField.allowsDefaultTighteningForTruncation = YES;

			NSFont *font = [NSFont monospacedDigitSystemFontOfSize:fontSize weight:NSFontWeightRegular];

			cellView.textField.font = font;
			cellView.textField.stringValue = cellText;
			cellView.textField.alignment = cellTextAlignment;

			if(cellView.textField.intrinsicContentSize.width > cellView.textField.frame.size.width - 4) {
				cellView.textField.toolTip = cellText;
			}
		}
		if(cellButton) {
			[cellView setSubviews:@[cellButton]];
		}
	}

	return view;
}

- (IBAction)applyPaths:(id)sender {
	for(PathItem *pi in [pathsList arrangedObjects]) {
		if(pi.enabled) {
			NSOpenPanel *panel = [NSOpenPanel openPanel];
			[panel setAllowsMultipleSelection:NO];
			[panel setCanChooseDirectories:YES];
			[panel setCanChooseFiles:NO];
			[panel setFloatingPanel:YES];
			[panel setDirectoryURL:[NSURL fileURLWithPath:pi.path]];
			[panel setTitle:@"Open to add path"];
			NSInteger result = [panel runModal];
			if(result == NSModalResponseOK) {
				[sandboxPathBehaviorController addUrl:[panel URL]];
			}
		}
	}
	[[self window] orderOut:self];
}

@end
