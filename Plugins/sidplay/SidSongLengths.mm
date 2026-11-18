//
//  SidSongLengths.mm
//  sidplay
//
//  Song lengths from the High Voltage SID Collection's database.
//

#import "SidSongLengths.h"

#import "SandboxBroker.h"

#import "Logging.h"

// How far above a tune the collection's root is looked for. The collection
// itself is no more than five folders deep.
static const int kMaxLevels = 8;

@implementation SidSongLengths

static NSLock *lock;
// Folder path to the path of the database found for it, or NSNull.
static NSMutableDictionary<NSString *, id> *databasePaths;
// Database path to its entries.
static NSMutableDictionary<NSString *, NSDictionary<NSString *, NSString *> *> *databases;

+ (void)initialize {
	if(self == [SidSongLengths class]) {
		lock = [NSLock new];
		databasePaths = [NSMutableDictionary new];
		databases = [NSMutableDictionary new];
	}
}

+ (NSArray<NSNumber *> *)lengthsFromString:(NSString *)string {
	NSMutableArray<NSNumber *> *lengths = [NSMutableArray array];
	for(NSString *field in [string componentsSeparatedByCharactersInSet:[NSCharacterSet whitespaceCharacterSet]]) {
		if(![field length]) continue;
		// "m:ss" or "m:ss.SSS"; older databases follow it with "(G)" or the
		// like, which the scanner leaves alone.
		NSScanner *scanner = [NSScanner scannerWithString:field];
		[scanner setCharactersToBeSkipped:nil];
		NSInteger minutes = 0;
		double seconds = 0.0;
		if([scanner scanInteger:&minutes] && [scanner scanString:@":" intoString:NULL] && [scanner scanDouble:&seconds] && minutes >= 0 && seconds >= 0.0) {
			[lengths addObject:@(minutes * 60.0 + seconds)];
		} else {
			[lengths addObject:@(0.0)];
		}
	}
	return lengths;
}

+ (NSDictionary<NSString *, NSString *> *)entriesFromData:(NSData *)data {
	NSMutableDictionary<NSString *, NSString *> *entries = [NSMutableDictionary dictionary];
	const char *line = (const char *)[data bytes];
	const char *end = line + [data length];
	while(line < end) {
		const char *next = line;
		while(next < end && *next != '\r' && *next != '\n')
			++next;
		// "0123…cdef=0:56 4:33.108"; comments start with ";", sections
		// with "[".
		if(next - line > SidTune::MD5_LENGTH + 1 && line[SidTune::MD5_LENGTH] == '=') {
			NSString *md5 = [[NSString alloc] initWithBytes:line length:SidTune::MD5_LENGTH encoding:NSASCIIStringEncoding];
			NSString *lengths = [[NSString alloc] initWithBytes:line + SidTune::MD5_LENGTH + 1 length:next - line - SidTune::MD5_LENGTH - 1 encoding:NSASCIIStringEncoding];
			if(md5 && lengths)
				entries[[md5 lowercaseString]] = lengths;
		}
		line = next + 1;
	}
	return entries;
}

// The database for tunes in `folder`, looked for in it and above it. Call
// with the lock held, and with access to the folder.
+ (NSDictionary<NSString *, NSString *> *)databaseForFolder:(NSString *)folder {
	NSFileManager *fileManager = [NSFileManager defaultManager];
	NSMutableArray<NSString *> *visited = [NSMutableArray array];
	id found = nil;

	NSString *at = folder;
	for(int level = 0; level < kMaxLevels && [at length] > 1; ++level) {
		found = databasePaths[at];
		if(found) break;
		[visited addObject:at];
		for(NSString *candidate in @[@"DOCUMENTS/Songlengths.md5", @"Songlengths.md5"]) {
			NSString *path = [at stringByAppendingPathComponent:candidate];
			if([fileManager isReadableFileAtPath:path]) {
				found = path;
				break;
			}
		}
		if(found) break;
		at = [at stringByDeletingLastPathComponent];
	}
	if(!found) found = [NSNull null];

	for(NSString *path in visited) {
		databasePaths[path] = found;
	}
	if(found == [NSNull null]) return nil;

	NSDictionary<NSString *, NSString *> *database = databases[found];
	if(!database) {
		NSData *data = [NSData dataWithContentsOfFile:found options:NSDataReadingMappedIfSafe error:nil];
		database = data ? [self entriesFromData:data] : @{};
		databases[found] = database;
		DLog(@"SID: Read %lu song length entries from %@", (unsigned long)[database count], found);
	}
	return database;
}

+ (NSArray<NSNumber *> *)lengthsForTune:(SidTune *)tune url:(NSURL *)url {
	if(!tune || ![url isFileURL]) return nil;

	char md5[SidTune::MD5_LENGTH + 1];
	if(!tune->createMD5New(md5)) return nil;
	NSString *key = [@(md5) lowercaseString];
	if([key length] != SidTune::MD5_LENGTH) return nil;

	NSString *folder = [[url path] stringByDeletingLastPathComponent];
	if(![folder length]) return nil;

	NSString *lengths = nil;
	BOOL looked = NO;
	[lock lock];
	id known = databasePaths[folder];
	if(known == [NSNull null]) {
		// Nothing above this folder, as found before.
		looked = YES;
	} else if(known && databases[known]) {
		lengths = databases[known][key];
		looked = YES;
	}
	[lock unlock];

	if(!looked) {
		// The database is above the tune, so only a folder the user gave
		// Cog as a whole, or one above it, reaches both. Access is asked
		// for outside the lock, as the broker answers on the main thread.
		id sandboxBrokerClass = NSClassFromString(@"SandboxBroker");
		id sandboxBroker = [sandboxBrokerClass sharedSandboxBroker];
		const void *handle = [sandboxBroker beginFolderAccess:url];
		[lock lock];
		lengths = [self databaseForFolder:folder][key];
		[lock unlock];
		[sandboxBroker endFolderAccess:handle];
	}

	return lengths ? [self lengthsFromString:lengths] : nil;
}

+ (double)lengthOfSong:(unsigned int)song ofTune:(SidTune *)tune url:(NSURL *)url {
	NSArray<NSNumber *> *lengths = [self lengthsForTune:tune url:url];
	if(song < 1 || song > [lengths count]) return 0.0;
	return [lengths[song - 1] doubleValue];
}

@end
