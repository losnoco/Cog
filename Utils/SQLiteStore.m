//
//  SQLiteStore.m
//  Cog
//
//  Created by Christopher Snowhill on 12/22/21.
//

#import <Foundation/Foundation.h>

#import <CoreData/CoreData.h>

#import "SQLiteStore.h"
#import "Logging.h"

#import "SHA256Digest.h"

extern NSPersistentContainer *kPersistentContainer;

NSString *getDatabasePath(void) {
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
	NSString *basePath = [[paths firstObject] stringByAppendingPathComponent:@"Cog"];
	NSString *filename = @"Default.sqlite";
	return [basePath stringByAppendingPathComponent:filename];
}

static int64_t currentSchemaVersion = 3;

NSArray *createSchema(void) {
	return @[
		@"CREATE TABLE IF NOT EXISTS stringdictionary ( \
        stringid INTEGER PRIMARY KEY AUTOINCREMENT, \
        referencecount INTEGER, \
        value TEXT NOT NULL \
    );",
		@"CREATE TABLE IF NOT EXISTS artdictionary ( \
        artid INTEGER PRIMARY KEY AUTOINCREMENT, \
		arthash BLOB NOT NULL, \
        referencecount INTEGER, \
        value BLOB NOT NULL \
    );",
		@"CREATE UNIQUE INDEX idx_art_hash ON artdictionary (arthash);",
		@"CREATE TABLE IF NOT EXISTS knowntracks ( \
        trackid INTEGER PRIMARY KEY AUTOINCREMENT, \
        referencecount INTEGER, \
        urlid INTEGER, \
        artid INTEGER, \
        albumid INTEGER, \
        albumartistid INTEGER, \
        artistid INTEGER, \
        titleid INTEGER, \
        genreid INTEGER, \
        codecid INTEGER, \
        encodingid INTEGER, \
        cuesheetid INTEGER, \
        track INTEGER, \
        year INTEGER, \
        unsigned INTEGER, \
        bitrate INTEGER, \
        samplerate REAL, \
        bitspersample INTEGER, \
        channels INTEGER, \
        channelconfig INTEGER, \
        endianid INTEGER, \
        floatingpoint INTEGER, \
        totalframes INTEGER, \
        metadataloaded INTEGER, \
        seekable INTEGER, \
        volume REAL, \
        replaygainalbumgain REAL, \
        replaygainalbumpeak REAL, \
        replaygaintrackgain REAL, \
        replaygaintrackpeak REAL \
    );",
		@"CREATE TABLE IF NOT EXISTS playlist ( \
        entryid INTEGER PRIMARY KEY AUTOINCREMENT, \
        entryindex INTEGER, \
        trackid INTEGER \
    );",
		@"CREATE TABLE IF NOT EXISTS queue ( \
        queueid INTEGER PRIMARY KEY AUTOINCREMENT, \
        queueindex INTEGER, \
        entryid INTEGER \
    );"
	];
}

enum {
	stmt_user_version_get = 0,

	stmt_select_string,
	stmt_select_string_refcount,
	stmt_select_string_value,
	stmt_bump_string,
	stmt_pop_string,
	stmt_add_string,
	stmt_remove_string,

	stmt_select_art,
	stmt_select_art_all,
	stmt_select_art_refcount,
	stmt_select_art_value,
	stmt_bump_art,
	stmt_pop_art,
	stmt_add_art,
	stmt_remove_art,
	stmt_add_art_renamed,

	stmt_select_track,
	stmt_select_track_refcount,
	stmt_select_track_data,
	stmt_bump_track,
	stmt_pop_track,
	stmt_add_track,
	stmt_remove_track,
	stmt_update_track,

	stmt_select_playlist,
	stmt_select_playlist_range,
	stmt_select_playlist_all,
	stmt_increment_playlist_for_insert,
	stmt_decrement_playlist_for_removal,
	stmt_add_playlist,
	stmt_remove_playlist_by_range,
	stmt_count_playlist,
	stmt_update_playlist,

	stmt_select_queue,
	stmt_select_queue_by_playlist_entry,
	stmt_decrement_queue_for_removal,
	stmt_add_queue,
	stmt_remove_queue_by_index,
	stmt_remove_queue_all,
	stmt_count_queue,

	stmt_count,
};

enum {
	user_version_get_out_version_number = 0,
};

const char *query_user_version_get = "PRAGMA user_version";

enum {
	select_string_in_id = 1,

	select_string_out_string_id = 0,
	select_string_out_reference_count,
};

const char *query_select_string = "SELECT stringid, referencecount FROM stringdictionary WHERE (value = ?) LIMIT 1";

enum {
	select_string_refcount_in_id = 1,

	select_string_refcount_out_string_id = 0,
};

const char *query_select_string_refcount = "SELECT referencecount FROM stringdictionary WHERE (stringid = ?) LIMIT 1";

enum {
	select_string_value_in_id = 1,

	select_string_value_out_value = 0,
};

const char *query_select_string_value = "SELECT value FROM stringdictionary WHERE (stringid = ?) LIMIT 1";

enum {
	bump_string_in_id = 1,
};

const char *query_bump_string = "UPDATE stringdictionary SET referencecount = referencecount + 1 WHERE (stringid = ?) LIMIT 1";

enum {
	pop_string_in_id = 1,
};

const char *query_pop_string = "UPDATE stringdictionary SET referencecount = referencecount - 1 WHERE (stringid = ?) LIMIT 1";

enum {
	add_string_in_value = 1,
};

const char *query_add_string = "INSERT INTO stringdictionary (referencecount, value) VALUES (1, ?)";

enum {
	remove_string_in_id = 1,
};

const char *query_remove_string = "DELETE FROM stringdictionary WHERE (stringid = ?)";

enum {
	select_art_in_arthash = 1,

	select_art_out_art_id = 0,
	select_art_out_reference_count,
};

const char *query_select_art = "SELECT artid, referencecount FROM artdictionary WHERE (arthash = ?) LIMIT 1";

enum {
	select_art_all_out_id = 0,
	select_art_all_out_referencecount,
	select_art_all_out_value,
};

const char *query_select_art_all = "SELECT artid, referencecount, value FROM artdictionary";

enum {
	select_art_refcount_in_id = 1,

	select_art_refcount_out_reference_count = 0,
};

const char *query_select_art_refcount = "SELECT referencecount FROM artdictionary WHERE (artid = ?) LIMIT 1";

enum {
	select_art_value_in_id = 1,

	select_art_value_out_value = 0,
};

const char *query_select_art_value = "SELECT value FROM artdictionary WHERE (artid = ?) LIMIT 1";

enum {
	bump_art_in_id = 1,
};

const char *query_bump_art = "UPDATE artdictionary SET referencecount = referencecount + 1 WHERE (artid = ?) LIMIT 1";

enum {
	pop_art_in_id = 1,
};

const char *query_pop_art = "UPDATE artdictionary SET referencecount = referencecount - 1 WHERE (artid = ?) LIMIT 1";

enum {
	add_art_in_hash = 1,
	add_art_in_value,
};

const char *query_add_art = "INSERT INTO artdictionary (referencecount, arthash, value) VALUES (1, ?, ?)";

enum {
	remove_art_in_id = 1,
};

const char *query_remove_art = "DELETE FROM artdictionary WHERE (artid = ?)";

enum {
	add_art_renamed_in_id = 1,
	add_art_renamed_in_referencecount,
	add_art_renamed_in_hash,
	add_art_renamed_in_value,
};

const char *query_add_art_renamed = "INSERT INTO artdictionary_v2 (artid, referencecount, arthash, value) VALUES (?, ?, ?, ?)";

enum {
	select_track_in_id = 1,

	select_track_out_track_id = 0,
	select_track_out_reference_count,
};

const char *query_select_track = "SELECT trackid, referencecount FROM knowntracks WHERE (urlid = ?) LIMIT 1";

enum {
	select_track_refcount_in_id = 1,

	select_track_refcount_out_reference_count = 0,
};

const char *query_select_track_refcount = "SELECT referencecount FROM knowntracks WHERE (trackid = ?) LIMIT 1";

enum {
	select_track_data_in_id = 1,

	select_track_data_out_url_id = 0,
	select_track_data_out_art_id,
	select_track_data_out_album_id,
	select_track_data_out_albumartist_id,
	select_track_data_out_artist_id,
	select_track_data_out_title_id,
	select_track_data_out_genre_id,
	select_track_data_out_codec_id,
	select_track_data_out_cuesheet_id,
	select_track_data_out_encoding_id,
	select_track_data_out_track,
	select_track_data_out_year,
	select_track_data_out_unsigned,
	select_track_data_out_bitrate,
	select_track_data_out_samplerate,
	select_track_data_out_bitspersample,
	select_track_data_out_channels,
	select_track_data_out_channelconfig,
	select_track_data_out_endian_id,
	select_track_data_out_floatingpoint,
	select_track_data_out_totalframes,
	select_track_data_out_metadataloaded,
	select_track_data_out_seekable,
	select_track_data_out_volume,
	select_track_data_out_replaygainalbumgain,
	select_track_data_out_replaygainalbumpeak,
	select_track_data_out_replaygaintrackgain,
	select_track_data_out_replaygaintrackpeak,
};

const char *query_select_track_data = "SELECT urlid, artid, albumid, albumartistid, artistid, titleid, genreid, codecid, cuesheetid, encodingid, track, year, unsigned, bitrate, samplerate, bitspersample, channels, channelconfig, endianid, floatingpoint, totalframes, metadataloaded, seekable, volume, replaygainalbumgain, replaygainalbumpeak, replaygaintrackgain, replaygaintrackpeak FROM knowntracks WHERE (trackid = ?) LIMIT 1";

enum {
	bump_track_in_id = 1,
};

const char *query_bump_track = "UPDATE knowntracks SET referencecount = referencecount + 1 WHERE (trackid = ?) LIMIT 1";

enum {
	pop_track_in_id = 1,
};

const char *query_pop_track = "UPDATE knowntracks SET referencecount = referencecount - 1 WHERE (trackid = ?) LIMIT 1";

enum {
	add_track_in_url_id = 1,
	add_track_in_art_id,
	add_track_in_album_id,
	add_track_in_albumartist_id,
	add_track_in_artist_id,
	add_track_in_title_id,
	add_track_in_genre_id,
	add_track_in_codec_id,
	add_track_in_cuesheet_id,
	add_track_in_encoding_id,
	add_track_in_track,
	add_track_in_year,
	add_track_in_unsigned,
	add_track_in_bitrate,
	add_track_in_samplerate,
	add_track_in_bitspersample,
	add_track_in_channels,
	add_track_in_channelconfig,
	add_track_in_endian_id,
	add_track_in_floatingpoint,
	add_track_in_totalframes,
	add_track_in_metadataloaded,
	add_track_in_seekable,
	add_track_in_volume,
	add_track_in_replaygainalbumgain,
	add_track_in_replaygainalbumpeak,
	add_track_in_replaygaintrackgain,
	add_track_in_replaygaintrackpeak,
};

const char *query_add_track = "INSERT INTO knowntracks (referencecount, urlid, artid, albumid, albumartistid, artistid, titleid, genreid, codecid, cuesheetid, encodingid, track, year, unsigned, bitrate, samplerate, bitspersample, channels, channelconfig, endianid, floatingpoint, totalframes, metadataloaded, seekable, volume, replaygainalbumgain, replaygainalbumpeak, replaygaintrackgain, replaygaintrackpeak) VALUES (1, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

enum {
	remove_track_in_id = 1,
};

const char *query_remove_track = "DELETE FROM knowntracks WHERE (trackid = ?)";

enum {
	update_track_in_url_id = 1,
	update_track_in_art_id,
	update_track_in_album_id,
	update_track_in_albumartist_id,
	update_track_in_artist_id,
	update_track_in_title_id,
	update_track_in_genre_id,
	update_track_in_codec_id,
	update_track_in_cuesheet_id,
	update_track_in_encoding_id,
	update_track_in_track,
	update_track_in_year,
	update_track_in_unsigned,
	update_track_in_bitrate,
	update_track_in_samplerate,
	update_track_in_bitspersample,
	update_track_in_channels,
	update_track_in_channelconfig,
	update_track_in_endian_id,
	update_track_in_floatingpoint,
	update_track_in_totalframes,
	update_track_in_metadataloaded,
	update_track_in_seekable,
	update_track_in_volume,
	update_track_in_replaygainalbumgain,
	update_track_in_replaygainalbumpeak,
	update_track_in_replaygaintrackgain,
	update_track_in_replaygaintrackpeak,
	update_track_in_id
};

const char *query_update_track = "UPDATE knowntracks SET urlid = ?, artid = ?, albumid = ?, albumartistid = ?, artistid = ?, titleid = ?, genreid = ?, codecid = ?, cuesheetid = ?, encodingid = ?, track = ?, year = ?, unsigned = ?, bitrate = ?, samplerate = ?, bitspersample = ?, channels = ?, channelconfig = ?, endianid = ?, floatingpoint = ?, totalframes = ?, metadataloaded = ?, seekable = ?, volume = ?, replaygainalbumgain = ?, replaygainalbumpeak = ?, replaygaintrackgain = ?, replaygaintrackpeak = ? WHERE trackid = ?";

enum {
	select_playlist_in_id = 1,

	select_playlist_out_entry_id = 0,
	select_playlist_out_track_id,
};

const char *query_select_playlist = "SELECT entryid, trackid FROM playlist WHERE (entryindex = ?)";

enum {
	select_playlist_range_in_id_low = 1,
	select_playlist_range_in_id_high,

	select_playlist_range_out_entry_id = 0,
	select_playlist_range_out_track_id,
};

const char *query_select_playlist_range = "SELECT entryid, trackid FROM playlist WHERE (entryindex BETWEEN ? AND ?) ORDER BY entryindex ASC";

enum {
	select_playlist_all_out_entry_id = 0,
	select_playlist_all_out_entry_index,
	select_playlist_all_out_track_id,
};

const char *query_select_playlist_all = "SELECT entryid, entryindex, trackid FROM playlist ORDER BY entryindex ASC";

enum {
	increment_playlist_for_insert_in_count = 1,
	increment_playlist_for_insert_in_index,
};

const char *query_increment_playlist_for_insert = "UPDATE playlist SET entryindex = entryindex + ? WHERE (entryindex >= ?)";

enum {
	decrement_playlist_for_removal_in_count = 1,
	decrement_playlist_for_removal_in_index,
};

const char *query_decrement_playlist_for_removal = "UPDATE playlist SET entryindex = entryindex - ? WHERE (entryindex >= ?)";

enum {
	add_playlist_in_entry_index = 1,
	add_playlist_in_track_id,
};

const char *query_add_playlist = "INSERT INTO playlist(entryindex, trackid) VALUES (?, ?)";

enum {
	remove_playlist_by_range_in_low = 1,
	remove_playlist_by_range_in_high,
};

const char *query_remove_playlist_by_range = "DELETE FROM playlist WHERE (entryindex BETWEEN ? AND ?)";

enum {
	count_playlist_out_count = 0,
};

const char *query_count_playlist = "SELECT COUNT(*) FROM playlist";

enum {
	update_playlist_in_entry_index = 1,
	update_playlist_in_track_id,
	update_playlist_in_id
};

const char *query_update_playlist = "UPDATE playlist SET entryindex = ?, trackid = ? WHERE (entryid = ?)";

enum {
	select_queue_in_id = 1,

	select_queue_out_queue_id = 0,
	select_queue_out_entry_id,
};

const char *query_select_queue = "SELECT queueid, entryid FROM queue WHERE (queueindex = ?) LIMIT 1";

enum {
	select_queue_by_playlist_entry_in_id = 1,

	select_queue_by_playlist_entry_out_queue_index = 0,
};

const char *query_select_queue_by_playlist_entry = "SELECT queueindex FROM queue WHERE (entryid = ?) LIMIT 1";

enum {
	decrement_queue_for_removal_in_index = 1,
};

const char *query_decrement_queue_for_removal = "UPDATE queue SET queueindex = queueindex - 1 WHERE (queueindex >= ?)";

enum {
	add_queue_in_queue_index = 1,
	add_queue_in_entry_id,
};

const char *query_add_queue = "INSERT INTO queue(queueindex, entryid) VALUES (?, ?)";

enum {
	remove_queue_by_index_in_queue_index = 1,
};

const char *query_remove_queue_by_index = "DELETE FROM queue WHERE (queueindex = ?)";

const char *query_remove_queue_all = "DELETE FROM queue";

enum {
	count_queue_out_count = 0,
};

const char *query_count_queue = "SELECT COUNT(*) FROM queue";

NSURL *_Nonnull urlForPath(NSString *_Nullable path);

@interface SQLiteStore (Private)
- (NSString *_Nullable)addString:(id _Nullable)string returnId:(int64_t *_Nonnull)stringId;
- (NSString *_Nonnull)getString:(int64_t)stringId;
- (void)removeString:(int64_t)stringId;
- (NSData *_Nullable)addArt:(id _Nullable)art returnId:(int64_t *_Nonnull)artId;
- (NSData *_Nonnull)getArt:(int64_t)artId;
- (void)removeArt:(int64_t)artId;
#if 0
- (int64_t)addTrack:(PlaylistEntry *_Nonnull)track;
#endif
- (PlaylistEntry *_Nonnull)getTrack:(int64_t)trackId;
#if 0
- (void)removeTrack:(int64_t)trackId;
#endif
@end

@implementation SQLiteStore

static SQLiteStore *g_sharedStore = nil;

+ (SQLiteStore *)sharedStore {
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		g_sharedStore = [self new];
	});

	return g_sharedStore;
}

+ (BOOL)databaseStarted {
	return g_sharedStore != nil;
}

@synthesize databasePath = g_databasePath;
@synthesize database = g_database;

- (id)init {
	self = [super init];

	if(self) {
		g_databasePath = getDatabasePath();

		memset(stmt, 0, sizeof(stmt));

		BOOL dbExists = NO;

		if([[NSFileManager defaultManager] fileExistsAtPath:g_databasePath])
			dbExists = YES;

		if(sqlite3_open([g_databasePath UTF8String], &g_database) == SQLITE_OK) {
			char *error = "";

			if(!dbExists) {
				NSArray *schemas = createSchema();

				for(NSString *schema in schemas) {
					if(sqlite3_exec(g_database, [schema UTF8String], NULL, NULL, &error) != SQLITE_OK) {
						DLog(@"SQLite error: %s", error);
						return nil;
					}
				}

				NSString *createVersion = [NSString stringWithFormat:@"PRAGMA user_version = %lld", currentSchemaVersion];

				if(sqlite3_exec(g_database, [createVersion UTF8String], NULL, NULL, &error)) {
					DLog(@"SQLite error: %s", error);
					return nil;
				}
			}

#define PREPARE(name) (sqlite3_prepare_v2(g_database, query_##name, (int)strlen(query_##name), &stmt[stmt_##name], NULL))

			if(PREPARE(user_version_get)) {
				DLog(@"SQlite error: %s", error);
				return nil;
			}

			sqlite3_stmt *st = stmt[stmt_user_version_get];
			if(sqlite3_reset(st) ||
			   sqlite3_step(st) != SQLITE_ROW) {
				return nil;
			}

			int64_t knownVersion = sqlite3_column_int64(st, user_version_get_out_version_number);

			sqlite3_reset(st);

			if(knownVersion < currentSchemaVersion) {
				switch(knownVersion) {
					case 0:
						// Schema 0 to 1: Add cuesheet and encoding text fields to the knowntracks table
						if(sqlite3_exec(g_database, "ALTER TABLE knowntracks ADD encodingid INTEGER; ALTER TABLE knowntracks ADD cuesheetid INTEGER", NULL, NULL, &error)) {
							DLog(@"SQLite error: %s", error);
							return nil;
						}
						// break;

					case 1:
						// Schema 1 to 2: Add channelconfig integer field to the knowntracks table
						if(sqlite3_exec(g_database, "ALTER TABLE knowntracks ADD channelconfig INTEGER", NULL, NULL, &error)) {
							DLog(@"SQLite error: %s", error);
							return nil;
						}
						// break;

					case 2:
						// Schema 2 to 3: Add arthash blob field to the artdictionary table, requires transmutation
						{
							if(sqlite3_exec(g_database, "CREATE TABLE IF NOT EXISTS artdictionary_v2 ( "
							                            "  artid INTEGER PRIMARY KEY AUTOINCREMENT, "
							                            "  arthash BLOB NOT NULL, "
							                            "  referencecount INTEGER, "
							                            "  value BLOB NOT NULL); "
							                            "CREATE UNIQUE INDEX idx_art_hash ON artdictionary_v2 (arthash);",
							                NULL, NULL, &error)) {
								DLog(@"SQLite error: %s", error);
								return nil;
							}

							if(PREPARE(select_art_all) ||
							   PREPARE(add_art_renamed))
								return nil;

							// Add the art hashes to the table
							st = stmt[stmt_select_art_all];
							sqlite3_stmt *sta = stmt[stmt_add_art_renamed];

							if(sqlite3_reset(st))
								return nil;

							while(sqlite3_step(st) == SQLITE_ROW) {
								int64_t artId = sqlite3_column_int64(st, select_art_all_out_id);
								int64_t referenceCount = sqlite3_column_int64(st, select_art_all_out_referencecount);
								const void *artBytes = sqlite3_column_blob(st, select_art_all_out_value);
								size_t artLength = sqlite3_column_bytes(st, select_art_all_out_value);
								Class shaClass = NSClassFromString(@"SHA256Digest");
								NSData *hash = [shaClass digestBytes:artBytes length:artLength];
								if(sqlite3_reset(sta) ||
								   sqlite3_bind_int64(sta, add_art_renamed_in_id, artId) ||
								   sqlite3_bind_int64(sta, add_art_renamed_in_referencecount, referenceCount) ||
								   sqlite3_bind_blob64(sta, add_art_renamed_in_hash, [hash bytes], [hash length], SQLITE_STATIC) ||
								   sqlite3_bind_blob64(sta, add_art_renamed_in_value, artBytes, artLength, SQLITE_STATIC) ||
								   sqlite3_step(sta) != SQLITE_DONE)
									return nil;
							}

							sqlite3_reset(sta);

							sqlite3_finalize(sta);
							sqlite3_finalize(st);

							stmt[stmt_select_art_all] = NULL;
							stmt[stmt_add_art_renamed] = NULL;

							if(sqlite3_exec(g_database, "PRAGMA foreign_keys=off; BEGIN TRANSACTION; DROP TABLE artdictionary; ALTER TABLE artdictionary_v2 RENAME TO artdictionary; COMMIT; PRAGMA foreign_keys=on;", NULL, NULL, &error)) {
								DLog(@"SQLite error: %s", error);
								return nil;
							}
						}

						// break;

					default:
						break;
				}

				NSString *updateVersion = [NSString stringWithFormat:@"PRAGMA user_version = %lld", currentSchemaVersion];

				if(sqlite3_exec(g_database, [updateVersion UTF8String], NULL, NULL, &error)) {
					DLog(@"SQLite error: %s", error);
					return nil;
				}
			}

			if(PREPARE(select_string) ||
			   PREPARE(select_string_refcount) ||
			   PREPARE(select_string_value) ||
			   PREPARE(bump_string) ||
			   PREPARE(pop_string) ||
			   PREPARE(add_string) ||
			   PREPARE(remove_string) ||

			   PREPARE(select_art) ||
			   PREPARE(select_art_refcount) ||
			   PREPARE(select_art_value) ||
			   PREPARE(bump_art) ||
			   PREPARE(pop_art) ||
			   PREPARE(add_art) ||
			   PREPARE(remove_art) ||

			   PREPARE(select_track) ||
			   PREPARE(select_track_refcount) ||
			   PREPARE(select_track_data) ||
			   PREPARE(bump_track) ||
			   PREPARE(pop_track) ||
			   PREPARE(add_track) ||
			   PREPARE(remove_track) ||
			   PREPARE(update_track) ||

			   PREPARE(select_playlist) ||
			   PREPARE(select_playlist_range) ||
			   PREPARE(select_playlist_all) ||
			   PREPARE(increment_playlist_for_insert) ||
			   PREPARE(decrement_playlist_for_removal) ||
			   PREPARE(add_playlist) ||
			   PREPARE(remove_playlist_by_range) ||
			   PREPARE(count_playlist) ||
			   PREPARE(update_playlist) ||

			   PREPARE(select_queue) ||
			   PREPARE(select_queue_by_playlist_entry) ||
			   PREPARE(decrement_queue_for_removal) ||
			   PREPARE(add_queue) ||
			   PREPARE(remove_queue_by_index) ||
			   PREPARE(remove_queue_all) ||
			   PREPARE(count_queue)) {
				return nil;
			}
#undef PREPARE
			size_t count = [self playlistGetCount];

			databaseMirror = [NSMutableArray new];
			artTable = [NSMutableDictionary new];
			stringTable = [NSMutableDictionary new];

			for(size_t i = 0; i < count; ++i) {
				PlaylistEntry *pe = [self playlistGetItem:i];
				if(pe) [databaseMirror addObject:pe];
			}

			return self;
		}
	}

	return nil;
}

- (void)shutdown {
	if(g_database) {
		for(size_t i = 0; i < stmt_count; ++i) {
			if(stmt[i]) sqlite3_finalize(stmt[i]);
		}
		sqlite3_close(g_database);
		g_database = NULL;
	}
}

- (void)dealloc {
	[self shutdown];
}

- (NSString *)addString:(id _Nullable)inputObj returnId:(int64_t *_Nonnull)stringId {
	*stringId = -1;

	if(!inputObj) {
		return nil;
	}

	NSString *string = nil;

	if([inputObj isKindOfClass:[NSString class]]) {
		string = (NSString *)inputObj;
	}

	if(!string || [string length] == 0) {
		return string;
	}

	const char *str = [string UTF8String];
	uint64_t len = strlen(str); // SQLite expects number of bytes, not characters

	sqlite3_stmt *st = stmt[stmt_select_string];

	if(sqlite3_reset(st) ||
	   sqlite3_bind_text64(st, select_string_in_id, str, len, SQLITE_STATIC, SQLITE_UTF8)) {
		return string;
	}

	int64_t returnId; /*, refcount;*/

	int rc = sqlite3_step(st);

	if(rc != SQLITE_DONE && rc != SQLITE_ROW) {
		return string;
	}

	if(rc == SQLITE_ROW) {
		returnId = sqlite3_column_int64(st, select_string_out_string_id);
		/*refcount = sqlite3_column_int64(st, select_string_out_reference_count);*/
	}

	sqlite3_reset(st);

	if(rc != SQLITE_ROW) {
		st = stmt[stmt_add_string];

		if(sqlite3_reset(st) ||
		   sqlite3_bind_text64(st, add_string_in_value, str, len, SQLITE_STATIC, SQLITE_UTF8) ||
		   sqlite3_step(st) != SQLITE_DONE ||
		   sqlite3_reset(st)) {
			return string;
		}

		returnId = sqlite3_last_insert_rowid(g_database);
		/*refcount = 1;*/

		[stringTable setObject:string forKey:[@(returnId) stringValue]];
	} else {
		st = stmt[stmt_bump_string];

		if(sqlite3_reset(st) ||
		   sqlite3_bind_int64(st, bump_string_in_id, returnId) ||
		   sqlite3_step(st) != SQLITE_DONE ||
		   sqlite3_reset(st)) {
			return string;
		}

		string = [stringTable objectForKey:[@(returnId) stringValue]];
	}

	*stringId = returnId;
	return string;
}

- (NSString *_Nonnull)getString:(int64_t)stringId {
	if(stringId < 0)
		return @"";

	NSString *ret = [stringTable objectForKey:[@(stringId) stringValue]];
	if(ret) return ret;

	sqlite3_stmt *st = stmt[stmt_select_string_value];

	if(sqlite3_reset(st) ||
	   sqlite3_bind_int64(st, select_string_value_in_id, stringId)) {
		return @"";
	}

	int rc = sqlite3_step(st);

	if(rc != SQLITE_ROW && rc != SQLITE_DONE) {
		sqlite3_reset(st);
		return @"";
	}

	ret = @"";

	if(rc == SQLITE_ROW) {
		const unsigned char *str = sqlite3_column_text(st, select_string_value_out_value);
		int strBytes = sqlite3_column_bytes(st, select_string_value_out_value);
		if(str && strBytes && *str) {
			ret = [NSString stringWithUTF8String:(const char *)str];
		}
	}

	sqlite3_reset(st);

	[stringTable setObject:ret forKey:[@(stringId) stringValue]];

	return ret;
}

- (void)removeString:(int64_t)stringId {
	if(stringId < 0)
		return;

	int64_t refcount;

	sqlite3_stmt *st = stmt[stmt_select_string_refcount];

	if(sqlite3_reset(st) ||
	   sqlite3_bind_int64(st, select_string_refcount_in_id, stringId)) {
		return;
	}

	int rc = sqlite3_step(st);

	if(rc != SQLITE_ROW && rc != SQLITE_DONE) {
		return;
	}

	if(rc == SQLITE_ROW) {
		refcount = sqlite3_column_int64(st, select_string_refcount_out_string_id);
	}

	sqlite3_reset(st);

	if(rc != SQLITE_ROW) {
		refcount = 1;
	}

	if(refcount <= 1) {
		st = stmt[stmt_remove_string];

		if(sqlite3_reset(st) ||
		   sqlite3_bind_int64(st, remove_string_in_id, stringId) ||
		   sqlite3_step(st) != SQLITE_DONE ||
		   sqlite3_reset(st)) {
			return;
		}

		[stringTable removeObjectForKey:[@(stringId) stringValue]];
	} else {
		st = stmt[stmt_pop_string];

		if(sqlite3_reset(st) ||
		   sqlite3_bind_int64(st, pop_string_in_id, stringId) ||
		   sqlite3_step(st) != SQLITE_DONE ||
		   sqlite3_reset(st)) {
			return;
		}
	}
}

- (NSData *_Nullable)addArt:(id _Nullable)inputObj returnId:(int64_t *_Nonnull)artId {
	*artId = -1;

	if(!inputObj) {
		return nil;
	}

	NSData *art = nil;

	if([inputObj isKindOfClass:[NSData class]]) {
		art = (NSData *)inputObj;
	}

	if(!art || [art length] == 0) {
		return art;
	}

	Class shaClass = NSClassFromString(@"SHA256Digest");
	NSData *digest = [shaClass digestData:art];

	sqlite3_stmt *st = stmt[stmt_select_art];

	if(sqlite3_reset(st) ||
	   sqlite3_bind_blob64(st, select_art_in_arthash, [digest bytes], [digest length], SQLITE_STATIC)) {
		return art;
	}

	int64_t returnId; /*, refcount;*/

	int rc = sqlite3_step(st);

	if(rc != SQLITE_DONE && rc != SQLITE_ROW) {
		return art;
	}

	if(rc == SQLITE_ROW) {
		returnId = sqlite3_column_int64(st, select_art_out_art_id);
		/*refcount = sqlite3_column_int64(st, select_art_out_reference_count);*/
	}

	sqlite3_reset(st);

	if(rc != SQLITE_ROW) {
		st = stmt[stmt_add_art];

		if(sqlite3_reset(st) ||
		   sqlite3_bind_blob64(st, add_art_in_hash, [digest bytes], [digest length], SQLITE_STATIC) ||
		   sqlite3_bind_blob64(st, add_art_in_value, [art bytes], [art length], SQLITE_STATIC) ||
		   sqlite3_step(st) != SQLITE_DONE ||
		   sqlite3_reset(st)) {
			return art;
		}

		returnId = sqlite3_last_insert_rowid(g_database);
		/*refcount = 1;*/

		[artTable setObject:art forKey:[@(returnId) stringValue]];
	} else {
		st = stmt[stmt_bump_art];

		if(sqlite3_reset(st) ||
		   sqlite3_bind_int64(st, bump_art_in_id, returnId) ||
		   sqlite3_step(st) != SQLITE_DONE ||
		   sqlite3_reset(st)) {
			return art;
		}

		art = [artTable objectForKey:[@(returnId) stringValue]];
	}

	*artId = returnId;
	return art;
}

- (NSData *_Nonnull)getArt:(int64_t)artId {
	if(artId < 0)
		return [NSData data];

	NSData *ret = [artTable valueForKey:[@(artId) stringValue]];
	if(ret) return ret;

	sqlite3_stmt *st = stmt[stmt_select_art_value];

	if(sqlite3_reset(st) ||
	   sqlite3_bind_int64(st, select_art_value_in_id, artId)) {
		return [NSData data];
	}

	int rc = sqlite3_step(st);

	if(rc != SQLITE_ROW && rc != SQLITE_DONE) {
		sqlite3_reset(st);
		return [NSData data];
	}

	ret = [NSData data];

	if(rc == SQLITE_ROW) {
		const void *blob = sqlite3_column_blob(st, select_art_value_out_value);
		int blobBytes = sqlite3_column_bytes(st, select_art_value_out_value);
		if(blob && blobBytes) {
			ret = [NSData dataWithBytes:blob length:blobBytes];
		}
	}

	sqlite3_reset(st);

	[artTable setValue:ret forKey:[@(artId) stringValue]];

	return ret;
}

- (void)removeArt:(int64_t)artId {
	if(artId < 0)
		return;

	int64_t refcount;

	sqlite3_stmt *st = stmt[stmt_select_art_refcount];

	if(sqlite3_reset(st) ||
	   sqlite3_bind_int64(st, select_art_refcount_in_id, artId)) {
		return;
	}

	int rc = sqlite3_step(st);

	if(rc != SQLITE_ROW && rc != SQLITE_DONE) {
		return;
	}

	if(rc == SQLITE_ROW) {
		refcount = sqlite3_column_int64(st, select_art_refcount_out_reference_count);
	}

	sqlite3_reset(st);

	if(rc != SQLITE_ROW) {
		refcount = 1;
	}

	if(refcount <= 1) {
		[artTable removeObjectForKey:[@(artId) stringValue]];

		st = stmt[stmt_remove_art];

		if(sqlite3_reset(st) ||
		   sqlite3_bind_int64(st, remove_art_in_id, artId) ||
		   sqlite3_step(st) != SQLITE_DONE ||
		   sqlite3_reset(st)) {
			return;
		}
	} else {
		st = stmt[stmt_pop_art];

		if(sqlite3_reset(st) ||
		   sqlite3_bind_int64(st, pop_art_in_id, artId) ||
		   sqlite3_step(st) != SQLITE_DONE ||
		   sqlite3_reset(st)) {
			return;
		}
	}
}

- (int64_t)addTrack:(PlaylistEntry *_Nonnull)track {
	NSURL *url = track.url;
	NSString *urlString = [url absoluteString];

	int64_t urlId = -1;
	[self addString:urlString returnId:&urlId];

	sqlite3_stmt *st = stmt[stmt_select_track];

	if(sqlite3_reset(st) ||
	   sqlite3_bind_int64(st, select_track_in_id, urlId)) {
		[self removeString:urlId];
		return -1;
	}

	int64_t ret/*, refcount*/;

	int rc = sqlite3_step(st);

	if(rc != SQLITE_DONE && rc != SQLITE_ROW) {
		[self removeString:urlId];
		return -1;
	}

	if(rc == SQLITE_ROW) {
		ret = sqlite3_column_int64(st, select_track_out_track_id);
		/*refcount = sqlite3_column_int64(st, select_track_out_reference_count);*/
	}

	sqlite3_reset(stmt[stmt_select_string]);

	if(rc != SQLITE_ROW) {
		NSString *temp;
		int64_t albumId = -1;
		temp = [self addString:track.album returnId:&albumId];
		if(temp) track.album = temp;
		int64_t albumartistId = -1;
		temp = [self addString:track.albumartist returnId:&albumartistId];
		if(temp) track.albumartist = temp;
		int64_t artistId = -1;
		temp = [self addString:track.artist returnId:&artistId];
		if(temp) track.artist = temp;
		int64_t titleId = -1;
		temp = [self addString:track.rawTitle returnId:&titleId];
		if(temp) track.rawTitle = temp;
		int64_t genreId = -1;
		temp = [self addString:track.genre returnId:&genreId];
		if(temp) track.genre = temp;
		int64_t codecId = -1;
		temp = [self addString:track.codec returnId:&codecId];
		if(temp) track.codec = temp;
		int64_t cuesheetId = -1;
		temp = [self addString:track.cuesheet returnId:&cuesheetId];
		if(temp) track.cuesheet = temp;
		int64_t encodingId = -1;
		temp = [self addString:track.encoding returnId:&encodingId];
		if(temp) track.encoding = temp;
		int64_t trackNr = track.track | (((uint64_t)track.disc) << 32);
		int64_t year = track.year;
		int64_t unsignedFmt = track.unSigned;
		int64_t bitrate = track.bitrate;
		double samplerate = track.sampleRate;
		int64_t bitspersample = track.bitsPerSample;
		int64_t channels = track.channels;
		int64_t channelConfig = track.channelConfig;
		int64_t endianId = -1;
		temp = [self addString:track.endian returnId:&endianId];
		if(temp) track.endian = temp;
		int64_t floatingpoint = track.floatingPoint;
		int64_t totalframes = track.totalFrames;
		int64_t metadataloaded = track.metadataLoaded;
		int64_t seekable = track.seekable;
		double volume = track.volume;
		double replaygainalbumgain = track.replayGainAlbumGain;
		double replaygainalbumpeak = track.replayGainAlbumPeak;
		double replaygaintrackgain = track.replayGainTrackGain;
		double replaygaintrackpeak = track.replayGainTrackPeak;

		NSData *albumArt = track.albumArtInternal;
		int64_t artId = -1;

		if(albumArt)
			albumArt = [self addArt:albumArt returnId:&artId];
		if(albumArt) track.albumArtInternal = albumArt;

		st = stmt[stmt_add_track];

		if(sqlite3_reset(st) ||
		   sqlite3_bind_int64(st, add_track_in_url_id, urlId) ||
		   sqlite3_bind_int64(st, add_track_in_art_id, artId) ||
		   sqlite3_bind_int64(st, add_track_in_album_id, albumId) ||
		   sqlite3_bind_int64(st, add_track_in_albumartist_id, albumartistId) ||
		   sqlite3_bind_int64(st, add_track_in_artist_id, artistId) ||
		   sqlite3_bind_int64(st, add_track_in_title_id, titleId) ||
		   sqlite3_bind_int64(st, add_track_in_genre_id, genreId) ||
		   sqlite3_bind_int64(st, add_track_in_codec_id, codecId) ||
		   sqlite3_bind_int64(st, add_track_in_cuesheet_id, cuesheetId) ||
		   sqlite3_bind_int64(st, add_track_in_encoding_id, encodingId) ||
		   sqlite3_bind_int64(st, add_track_in_track, trackNr) ||
		   sqlite3_bind_int64(st, add_track_in_year, year) ||
		   sqlite3_bind_int64(st, add_track_in_unsigned, unsignedFmt) ||
		   sqlite3_bind_int64(st, add_track_in_bitrate, bitrate) ||
		   sqlite3_bind_double(st, add_track_in_samplerate, samplerate) ||
		   sqlite3_bind_int64(st, add_track_in_bitspersample, bitspersample) ||
		   sqlite3_bind_int64(st, add_track_in_channels, channels) ||
		   sqlite3_bind_int64(st, add_track_in_channelconfig, channelConfig) ||
		   sqlite3_bind_int64(st, add_track_in_endian_id, endianId) ||
		   sqlite3_bind_int64(st, add_track_in_floatingpoint, floatingpoint) ||
		   sqlite3_bind_int64(st, add_track_in_totalframes, totalframes) ||
		   sqlite3_bind_int64(st, add_track_in_metadataloaded, metadataloaded) ||
		   sqlite3_bind_int64(st, add_track_in_seekable, seekable) ||
		   sqlite3_bind_double(st, add_track_in_volume, volume) ||
		   sqlite3_bind_double(st, add_track_in_replaygainalbumgain, replaygainalbumgain) ||
		   sqlite3_bind_double(st, add_track_in_replaygainalbumpeak, replaygainalbumpeak) ||
		   sqlite3_bind_double(st, add_track_in_replaygaintrackgain, replaygaintrackgain) ||
		   sqlite3_bind_double(st, add_track_in_replaygaintrackpeak, replaygaintrackpeak) ||
		   sqlite3_step(st) != SQLITE_DONE ||
		   sqlite3_reset(st)) {
			[self removeArt:artId];

			[self removeString:albumId];
			[self removeString:albumartistId];
			[self removeString:artistId];
			[self removeString:titleId];
			[self removeString:genreId];
			[self removeString:codecId];
			[self removeString:cuesheetId];
			[self removeString:encodingId];
			[self removeString:endianId];

			[self removeString:urlId];

			return -1;
		}

		ret = sqlite3_last_insert_rowid(g_database);
		/*refcount = 1;*/
	} else {
		[self removeString:urlId]; // should only be bumped once per instance of track

		st = stmt[stmt_bump_track];

		if(sqlite3_reset(st) ||
		   sqlite3_bind_int64(st, bump_track_in_id, ret) ||
		   sqlite3_step(st) != SQLITE_DONE ||
		   sqlite3_reset(st)) {
			return -1;
		}
	}

	[track setDbIndex:ret];

	return ret;
}

#if 0
- (void)trackUpdate:(PlaylistEntry *)track {
	NSURL *url = track.url;
	NSString *urlString = [url absoluteString];

	int64_t urlId = -1;
	[self addString:urlString returnId:&urlId];

	sqlite3_stmt *st = stmt[stmt_select_track];

	if(sqlite3_reset(st) ||
	   sqlite3_bind_int64(st, select_track_in_id, urlId)) {
		[self removeString:urlId];
		return;
	}

	int64_t trackId; /*, refcount; */

	int rc = sqlite3_step(st);

	if(rc != SQLITE_DONE && rc != SQLITE_ROW) {
		[self removeString:urlId];
		return;
	}

	trackId = -1;

	if(rc == SQLITE_ROW) {
		trackId = sqlite3_column_int64(st, select_track_out_track_id);
		/*refcount = sqlite3_column_int64(st, select_track_out_reference_count);*/
	}

	sqlite3_reset(stmt[stmt_select_string]);

	if(trackId < 0) {
		[self removeString:urlId];
		return;
	}

	if(trackId != [track dbIndex] && [track dbIndex] != 0) {
		[self removeString:urlId];
		return;
	}

	if([track dbIndex] == 0) {
		[track setDbIndex:trackId];
	}

	st = stmt[stmt_select_track_data];

	if(sqlite3_reset(st) ||
	   sqlite3_bind_int64(st, select_track_data_in_id, trackId)) {
		[self removeString:urlId];
		return;
	}

	rc = sqlite3_step(st);

	if(rc != SQLITE_ROW && rc != SQLITE_DONE) {
		sqlite3_reset(st);
		[self removeString:urlId];
		return;
	}

	if(rc == SQLITE_ROW) {
		int64_t urlId = sqlite3_column_int64(st, select_track_data_out_url_id);
		int64_t artId = sqlite3_column_int64(st, select_track_data_out_art_id);
		int64_t albumId = sqlite3_column_int64(st, select_track_data_out_album_id);
		int64_t albumartistId = sqlite3_column_int64(st, select_track_data_out_albumartist_id);
		int64_t artistId = sqlite3_column_int64(st, select_track_data_out_artist_id);
		int64_t titleId = sqlite3_column_int64(st, select_track_data_out_title_id);
		int64_t genreId = sqlite3_column_int64(st, select_track_data_out_genre_id);
		int64_t codecId = sqlite3_column_int64(st, select_track_data_out_codec_id);
		int64_t cuesheetId = sqlite3_column_int64(st, select_track_data_out_cuesheet_id);
		int64_t encodingId = sqlite3_column_int64(st, select_track_data_out_encoding_id);
		int64_t endianId = sqlite3_column_int64(st, select_track_data_out_endian_id);

		[self removeArt:artId];

		[self removeString:urlId];
		[self removeString:albumId];
		[self removeString:albumartistId];
		[self removeString:artistId];
		[self removeString:titleId];
		[self removeString:genreId];
		[self removeString:codecId];
		[self removeString:cuesheetId];
		[self removeString:encodingId];
		[self removeString:endianId];
	}

	sqlite3_reset(st);

	{
		NSString *temp;
		int64_t albumId = -1;
		temp = [self addString:track.album returnId:&albumId];
		if(temp) track.album = temp;
		int64_t albumartistId = -1;
		temp = [self addString:track.albumartist returnId:&albumartistId];
		if(temp) track.albumartist = temp;
		int64_t artistId = -1;
		temp = [self addString:track.artist returnId:&artistId];
		if(temp) track.artist = temp;
		int64_t titleId = -1;
		temp = [self addString:track.rawTitle returnId:&titleId];
		if(temp) track.rawTitle = temp;
		int64_t genreId = -1;
		temp = [self addString:track.genre returnId:&genreId];
		if(temp) track.genre = temp;
		int64_t codecId = -1;
		temp = [self addString:track.codec returnId:&codecId];
		if(temp) track.codec = temp;
		int64_t cuesheetId = -1;
		temp = [self addString:track.cuesheet returnId:&cuesheetId];
		if(temp) track.cuesheet = temp;
		int64_t encodingId = -1;
		temp = [self addString:track.encoding returnId:&encodingId];
		if(temp) track.encoding = temp;
		int64_t trackNr = track.track | (((uint64_t)track.disc) << 32);
		int64_t year = track.year;
		int64_t unsignedFmt = track.unSigned;
		int64_t bitrate = track.bitrate;
		double samplerate = track.sampleRate;
		int64_t bitspersample = track.bitsPerSample;
		int64_t channels = track.channels;
		int64_t channelConfig = track.channelConfig;
		int64_t endianId = -1;
		temp = [self addString:track.endian returnId:&endianId];
		if(temp) track.endian = temp;
		int64_t floatingpoint = track.floatingPoint;
		int64_t totalframes = track.totalFrames;
		int64_t metadataloaded = track.metadataLoaded;
		int64_t seekable = track.seekable;
		double volume = track.volume;
		double replaygainalbumgain = track.replayGainAlbumGain;
		double replaygainalbumpeak = track.replayGainAlbumPeak;
		double replaygaintrackgain = track.replayGainTrackGain;
		double replaygaintrackpeak = track.replayGainTrackPeak;

		NSData *albumArt = track.albumArtInternal;
		int64_t artId = -1;

		if(albumArt)
			albumArt = [self addArt:albumArt returnId:&artId];
		if(albumArt) track.albumArtInternal = albumArt;

		st = stmt[stmt_update_track];

		if(sqlite3_reset(st) ||
		   sqlite3_bind_int64(st, update_track_in_url_id, urlId) ||
		   sqlite3_bind_int64(st, update_track_in_art_id, artId) ||
		   sqlite3_bind_int64(st, update_track_in_album_id, albumId) ||
		   sqlite3_bind_int64(st, update_track_in_albumartist_id, albumartistId) ||
		   sqlite3_bind_int64(st, update_track_in_artist_id, artistId) ||
		   sqlite3_bind_int64(st, update_track_in_title_id, titleId) ||
		   sqlite3_bind_int64(st, update_track_in_genre_id, genreId) ||
		   sqlite3_bind_int64(st, update_track_in_codec_id, codecId) ||
		   sqlite3_bind_int64(st, update_track_in_cuesheet_id, cuesheetId) ||
		   sqlite3_bind_int64(st, update_track_in_encoding_id, encodingId) ||
		   sqlite3_bind_int64(st, update_track_in_track, trackNr) ||
		   sqlite3_bind_int64(st, update_track_in_year, year) ||
		   sqlite3_bind_int64(st, update_track_in_unsigned, unsignedFmt) ||
		   sqlite3_bind_int64(st, update_track_in_bitrate, bitrate) ||
		   sqlite3_bind_double(st, update_track_in_samplerate, samplerate) ||
		   sqlite3_bind_int64(st, update_track_in_bitspersample, bitspersample) ||
		   sqlite3_bind_int64(st, update_track_in_channels, channels) ||
		   sqlite3_bind_int64(st, update_track_in_channelconfig, channelConfig) ||
		   sqlite3_bind_int64(st, update_track_in_endian_id, endianId) ||
		   sqlite3_bind_int64(st, update_track_in_floatingpoint, floatingpoint) ||
		   sqlite3_bind_int64(st, update_track_in_totalframes, totalframes) ||
		   sqlite3_bind_int64(st, update_track_in_metadataloaded, metadataloaded) ||
		   sqlite3_bind_int64(st, update_track_in_seekable, seekable) ||
		   sqlite3_bind_double(st, update_track_in_volume, volume) ||
		   sqlite3_bind_double(st, update_track_in_replaygainalbumgain, replaygainalbumgain) ||
		   sqlite3_bind_double(st, update_track_in_replaygainalbumpeak, replaygainalbumpeak) ||
		   sqlite3_bind_double(st, update_track_in_replaygaintrackgain, replaygaintrackgain) ||
		   sqlite3_bind_double(st, update_track_in_replaygaintrackpeak, replaygaintrackpeak) ||
		   sqlite3_bind_int64(st, update_track_in_id, trackId) ||
		   sqlite3_step(st) != SQLITE_DONE ||
		   sqlite3_reset(st)) {
			[self removeArt:artId];

			[self removeString:albumId];
			[self removeString:albumartistId];
			[self removeString:artistId];
			[self removeString:titleId];
			[self removeString:genreId];
			[self removeString:codecId];
			[self removeString:cuesheetId];
			[self removeString:encodingId];
			[self removeString:endianId];

			[self removeString:urlId];

			return;
		}

		[databaseMirror replaceObjectAtIndex:[track index] withObject:track];
	}
}
#endif

- (PlaylistEntry *_Nonnull)getTrack:(int64_t)trackId {
	__block PlaylistEntry *entry = nil;
	[kPersistentContainer.viewContext performBlockAndWait:^{
		entry = [NSEntityDescription insertNewObjectForEntityForName:@"PlaylistEntry" inManagedObjectContext:kPersistentContainer.viewContext];

		if(trackId < 0) {
			entry.error = YES;
			entry.errorMessage = NSLocalizedString(@"ErrorInvalidTrackId", @"");
			entry.deLeted = YES;
			return;
		}

		sqlite3_stmt *st = stmt[stmt_select_track_data];

		if(sqlite3_reset(st) ||
		   sqlite3_bind_int64(st, select_track_data_in_id, trackId)) {
			entry.error = YES;
			entry.errorMessage = NSLocalizedString(@"ErrorSqliteProblem", @"");
			entry.deLeted = YES;
			return;
		}

		int rc = sqlite3_step(st);

		if(rc != SQLITE_ROW && rc != SQLITE_DONE) {
			sqlite3_reset(st);
			entry.error = YES;
			entry.errorMessage = NSLocalizedString(@"ErrorSqliteProblem", @"");
			entry.deLeted = YES;
			return;
		}

		if(rc == SQLITE_ROW) {
			int64_t urlId = sqlite3_column_int64(st, select_track_data_out_url_id);
			int64_t artId = sqlite3_column_int64(st, select_track_data_out_art_id);
			int64_t albumId = sqlite3_column_int64(st, select_track_data_out_album_id);
			int64_t albumartistId = sqlite3_column_int64(st, select_track_data_out_albumartist_id);
			int64_t artistId = sqlite3_column_int64(st, select_track_data_out_artist_id);
			int64_t titleId = sqlite3_column_int64(st, select_track_data_out_title_id);
			int64_t genreId = sqlite3_column_int64(st, select_track_data_out_genre_id);
			int64_t codecId = sqlite3_column_int64(st, select_track_data_out_codec_id);
			int64_t cuesheetId = sqlite3_column_int64(st, select_track_data_out_cuesheet_id);
			int64_t encodingId = sqlite3_column_int64(st, select_track_data_out_encoding_id);
			int64_t trackNr = sqlite3_column_int64(st, select_track_data_out_track);
			int64_t year = sqlite3_column_int64(st, select_track_data_out_year);
			int64_t unsignedFmt = sqlite3_column_int64(st, select_track_data_out_unsigned);
			int64_t bitrate = sqlite3_column_int64(st, select_track_data_out_bitrate);
			double samplerate = sqlite3_column_double(st, select_track_data_out_samplerate);
			int64_t bitspersample = sqlite3_column_int64(st, select_track_data_out_bitspersample);
			int64_t channels = sqlite3_column_int64(st, select_track_data_out_channels);
			int64_t channelConfig = sqlite3_column_int64(st, select_track_data_out_channelconfig);
			int64_t endianId = sqlite3_column_int64(st, select_track_data_out_endian_id);
			int64_t floatingpoint = sqlite3_column_int64(st, select_track_data_out_floatingpoint);
			int64_t totalframes = sqlite3_column_int64(st, select_track_data_out_totalframes);
			int64_t metadataloaded = sqlite3_column_int64(st, select_track_data_out_metadataloaded);
			int64_t seekable = sqlite3_column_int64(st, select_track_data_out_seekable);
			double volume = sqlite3_column_double(st, select_track_data_out_volume);
			double replaygainalbumgain = sqlite3_column_double(st, select_track_data_out_replaygainalbumgain);
			double replaygainalbumpeak = sqlite3_column_double(st, select_track_data_out_replaygainalbumpeak);
			double replaygaintrackgain = sqlite3_column_double(st, select_track_data_out_replaygaintrackgain);
			double replaygaintrackpeak = sqlite3_column_double(st, select_track_data_out_replaygaintrackpeak);

			uint64_t discNr = ((uint64_t)trackNr) >> 32;
			trackNr &= (1UL << 32) - 1;

			entry.url = urlForPath([self getString:urlId]);

			entry.album = [self getString:albumId];
			entry.albumartist = [self getString:albumartistId];
			entry.artist = [self getString:artistId];
			entry.rawTitle = [self getString:titleId];
			entry.genre = [self getString:genreId];
			entry.codec = [self getString:codecId];
			entry.cuesheet = [self getString:cuesheetId];
			entry.encoding = [self getString:encodingId];
			entry.track = (int32_t)trackNr;
			entry.disc = (int32_t)discNr;
			entry.year = (int32_t)year;
			entry.unSigned = !!unsignedFmt;
			entry.bitrate = (int32_t)bitrate;
			entry.sampleRate = samplerate;
			entry.bitsPerSample = (int32_t)bitspersample;
			entry.channels = (int32_t)channels;
			entry.channelConfig = (uint32_t)channelConfig;
			entry.endian = [self getString:endianId];
			entry.floatingPoint = !!floatingpoint;
			entry.totalFrames = totalframes;
			entry.seekable = !!seekable;
			entry.volume = volume;
			entry.replayGainAlbumGain = replaygainalbumgain;
			entry.replayGainAlbumPeak = replaygainalbumpeak;
			entry.replayGainTrackGain = replaygaintrackgain;
			entry.replayGainTrackPeak = replaygaintrackpeak;

			entry.albumArtInternal = [self getArt:artId];

			entry.metadataLoaded = !!metadataloaded;

			entry.dbIndex = trackId;
		} else {
			entry.error = YES;
			entry.errorMessage = NSLocalizedString(@"ErrorTrackMissing", @"");
			entry.deLeted = YES;
		}

		sqlite3_reset(st);
	}];

	return entry;
}

- (void)removeTrack:(int64_t)trackId {
	if(trackId < 0)
		return;

	int64_t refcount;

	sqlite3_stmt *st = stmt[stmt_select_track_refcount];

	if(sqlite3_reset(st) ||
	   sqlite3_bind_int64(st, select_track_refcount_in_id, trackId)) {
		return;
	}

	int rc = sqlite3_step(st);

	if(rc != SQLITE_ROW && rc != SQLITE_DONE) {
		return;
	}

	if(rc == SQLITE_ROW) {
		refcount = sqlite3_column_int64(st, select_track_refcount_out_reference_count);
	}

	sqlite3_reset(st);

	if(rc != SQLITE_ROW) {
		refcount = 1;
	}

	if(refcount <= 1) {
		// DeRef the strings and art

		st = stmt[stmt_select_track_data];

		if(sqlite3_reset(st) == SQLITE_OK &&
		   sqlite3_bind_int64(st, select_track_data_in_id, trackId) == SQLITE_OK &&
		   sqlite3_step(st) == SQLITE_ROW) {
			int64_t urlId = sqlite3_column_int64(st, select_track_data_out_url_id);
			int64_t artId = sqlite3_column_int64(st, select_track_data_out_art_id);
			int64_t albumId = sqlite3_column_int64(st, select_track_data_out_album_id);
			int64_t albumartistId = sqlite3_column_int64(st, select_track_data_out_albumartist_id);
			int64_t artistId = sqlite3_column_int64(st, select_track_data_out_artist_id);
			int64_t titleId = sqlite3_column_int64(st, select_track_data_out_title_id);
			int64_t genreId = sqlite3_column_int64(st, select_track_data_out_genre_id);
			int64_t codecId = sqlite3_column_int64(st, select_track_data_out_codec_id);
			int64_t cuesheetId = sqlite3_column_int64(st, select_track_data_out_cuesheet_id);
			int64_t encodingId = sqlite3_column_int64(st, select_track_data_out_encoding_id);
			int64_t endianId = sqlite3_column_int64(st, select_track_data_out_endian_id);

			sqlite3_reset(st);

			[self removeArt:artId];

			[self removeString:urlId];
			[self removeString:albumId];
			[self removeString:albumartistId];
			[self removeString:artistId];
			[self removeString:titleId];
			[self removeString:genreId];
			[self removeString:codecId];
			[self removeString:cuesheetId];
			[self removeString:encodingId];
			[self removeString:endianId];
		}

		st = stmt[stmt_remove_track];

		if(sqlite3_reset(st) ||
		   sqlite3_bind_int64(st, remove_track_in_id, trackId) ||
		   sqlite3_step(st) != SQLITE_DONE ||
		   sqlite3_reset(st)) {
			return;
		}
	} else {
		st = stmt[stmt_pop_track];

		if(sqlite3_reset(st) ||
		   sqlite3_bind_int64(st, pop_track_in_id, trackId) ||
		   sqlite3_step(st) != SQLITE_DONE ||
		   sqlite3_reset(st)) {
			return;
		}
	}
}

- (void)playlistInsertTracks:(NSArray *)tracks atIndex:(int64_t)index progressCall:(void (^)(double))callback {
	if(!tracks) {
		callback(-1);
		return;
	}

	callback(0);

	sqlite3_stmt *st = stmt[stmt_increment_playlist_for_insert];

	if(sqlite3_reset(st) ||
	   sqlite3_bind_int64(st, increment_playlist_for_insert_in_count, [tracks count]) ||
	   sqlite3_bind_int64(st, increment_playlist_for_insert_in_index, index) ||
	   sqlite3_step(st) != SQLITE_DONE ||
	   sqlite3_reset(st)) {
		callback(-1);
		return;
	}

	callback(25);

	double progress = 25.0;
	double progressstep = [tracks count] ? 75.0 / (double)([tracks count]) : 0;

	st = stmt[stmt_add_playlist];

	for(PlaylistEntry *entry in tracks) {
		int64_t trackId = [self addTrack:entry];

		if(sqlite3_reset(st) ||
		   sqlite3_bind_int64(st, add_playlist_in_entry_index, index) ||
		   sqlite3_bind_int64(st, add_playlist_in_track_id, trackId) ||
		   sqlite3_step(st) != SQLITE_DONE) {
			callback(-1);
			return;
		}

		++index;

		progress += progressstep;
		callback(progress);
	}

	sqlite3_reset(st);

	callback(-1);
}

- (void)playlistInsertTracks:(NSArray *)tracks atObjectIndexes:(NSIndexSet *)indexes progressCall:(void (^)(double))callback {
	if(!tracks || !indexes) {
		callback(-1);
		return;
	}

	NSMutableArray *tracksCopy = [NSMutableArray new];
	for(PlaylistEntry *pe in tracks) {
		[tracksCopy addObject:pe];
	}

	[databaseMirror insertObjects:tracksCopy atIndexes:indexes];

	__block int64_t total_count = 0;
	[indexes enumerateRangesUsingBlock:^(NSRange range, BOOL *_Nonnull stop) {
		total_count += range.length;
	}];

	__block int64_t i = 0;

	__block double progress = 0;

	[indexes enumerateRangesUsingBlock:^(NSRange range, BOOL *_Nonnull stop) {
		double progresschunk = (double)range.length / (double)total_count;
		double progressbase = progress;
		NSRange trackRange = NSMakeRange(i, range.length);
		NSArray *trackSet = (i == 0 && range.length == [tracksCopy count]) ? tracksCopy : [tracksCopy subarrayWithRange:trackRange];
		[self playlistInsertTracks:trackSet
		                   atIndex:range.location
		              progressCall:^(double _progress) {
			              if(_progress < 0) return;
			              callback(progressbase + progresschunk * _progress);
		              }];
		i += range.length;
		progress += 100.0 * progresschunk;
		callback(progress);
	}];
	callback(-1);
}

#if 0
- (void)playlistRemoveTracks:(int64_t)index forCount:(int64_t)count progressCall:(void (^)(double))callback {
	if(!count) {
		callback(-1);
		return;
	}

	sqlite3_stmt *st = stmt[stmt_select_playlist_range];

	if(sqlite3_reset(st) ||
	   sqlite3_bind_int64(st, select_playlist_range_in_id_low, index) ||
	   sqlite3_bind_int64(st, select_playlist_range_in_id_high, index + count - 1)) {
		callback(-1);
		return;
	}

	callback(0);

	double progress = 0;
	double progressstep = 100.0 / ((double)count);

	int rc = sqlite3_step(st);

	while(rc == SQLITE_ROW) {
		int64_t trackId = sqlite3_column_int64(st, select_playlist_range_out_track_id);
		[self removeTrack:trackId];
		rc = sqlite3_step(st);
		progress += progressstep;
		callback(progress);
	}

	callback(100);

	sqlite3_reset(st);

	if(rc != SQLITE_DONE) {
		return;
	}

	st = stmt[stmt_remove_playlist_by_range];

	if(sqlite3_reset(st) ||
	   sqlite3_bind_int64(st, remove_playlist_by_range_in_low, index) ||
	   sqlite3_bind_int64(st, remove_playlist_by_range_in_high, index + count - 1) ||
	   sqlite3_step(st) != SQLITE_DONE ||
	   sqlite3_reset(st)) {
		callback(-1);
		return;
	}

	st = stmt[stmt_decrement_playlist_for_removal];

	if(sqlite3_reset(st) ||
	   sqlite3_bind_int64(st, decrement_playlist_for_removal_in_count, count) ||
	   sqlite3_bind_int64(st, decrement_playlist_for_removal_in_index, index + count) ||
	   sqlite3_step(st) != SQLITE_DONE ||
	   sqlite3_reset(st)) {
		callback(-1);
		return;
	}

	NSMutableArray *items = [NSMutableArray new];

	for(int64_t i = index, j = index + count; i < j; ++i) {
		[items addObject:@(i)];
	}

	[self queueRemovePlaylistItems:items];

	callback(-1);
}

- (void)playlistRemoveTracksAtIndexes:(NSIndexSet *)indexes progressCall:(void (^)(double))callback {
	if(!indexes) {
		callback(-1);
		return;
	}

	[databaseMirror removeObjectsAtIndexes:indexes];

	__block int64_t total_count = 0;

	[indexes enumerateRangesUsingBlock:^(NSRange range, BOOL *_Nonnull stop) {
		total_count += range.length;
	}];

	__block int64_t i = 0;

	__block double progress = 0;

	callback(progress);

	[indexes enumerateRangesUsingBlock:^(NSRange range, BOOL *_Nonnull stop) {
		double progresschunk = (double)range.length / (double)total_count;
		double progressbase = progress;
		[self playlistRemoveTracks:(range.location - i)
		                  forCount:range.length
		              progressCall:^(double _progress) {
			              if(_progress < 0) return;
			              callback(progressbase + progresschunk * _progress);
		              }];
		i += range.length;
		progress += 100.0 * progresschunk;
		callback(progress);
	}];
	callback(-1);
}
#endif

- (PlaylistEntry *)playlistGetCachedItem:(int64_t)index {
	if(index >= 0 && index < [databaseMirror count])
		return [databaseMirror objectAtIndex:index];
	else
		return nil;
}

- (PlaylistEntry *)playlistGetItem:(int64_t)index {
	__block PlaylistEntry *entry = nil;

	sqlite3_stmt *st = stmt[stmt_select_playlist];

	if(sqlite3_reset(st) ||
	   sqlite3_bind_int64(st, select_playlist_in_id, index)) {
		return entry;
	}

	int rc = sqlite3_step(st);

	if(rc != SQLITE_ROW && rc != SQLITE_DONE) {
		return entry;
	}

	if(rc == SQLITE_ROW) {
		int64_t trackId = sqlite3_column_int64(st, select_playlist_out_track_id);
		int64_t entryId = sqlite3_column_int64(st, select_playlist_out_entry_id);
		entry = [self getTrack:trackId];
		if(!entry.deLeted && !entry.error) {
			[kPersistentContainer.viewContext performBlockAndWait:^{
				entry.index = index;
				entry.entryId = entryId;
			}];
		} else {
			[kPersistentContainer.viewContext performBlockAndWait:^{
				[kPersistentContainer.viewContext deleteObject:entry];
				entry = nil;
			}];
		}
	}

	sqlite3_reset(st);

	return entry;
}

- (int64_t)playlistGetCount {
	sqlite3_stmt *st = stmt[stmt_count_playlist];

	if(sqlite3_reset(st) ||
	   sqlite3_step(st) != SQLITE_ROW) {
		return 0;
	}

	int64_t ret = sqlite3_column_int64(st, count_playlist_out_count);

	sqlite3_reset(st);

	return ret;
}

- (int64_t)playlistGetCountCached {
	return [databaseMirror count];
}

#if 0
- (void)playlistMoveObjectsInArrangedObjectsFromIndexes:(NSIndexSet *)indexSet toIndex:(NSUInteger)insertIndex progressCall:(void (^)(double))callback {
	__block NSUInteger rangeCount = 0;
	__block NSUInteger firstIndex = 0;
	[indexSet enumerateRangesUsingBlock:^(NSRange range, BOOL *_Nonnull stop) {
		if(++rangeCount == 1)
			firstIndex = range.location;
	}];

	if(rangeCount == 1 &&
	   (insertIndex >= firstIndex &&
	    insertIndex < firstIndex + [indexSet count])) // Null operation
		return;

	NSArray *objects = databaseMirror;
	NSUInteger index = [indexSet lastIndex];

	NSUInteger aboveInsertIndexCount = 0;
	id object;
	NSUInteger removeIndex;

	callback(0);
	double progress = 0;
	double progressstep = 100.0 / [indexSet count];

	while(NSNotFound != index) {
		if(index >= insertIndex) {
			removeIndex = index + aboveInsertIndexCount;
			aboveInsertIndexCount += 1;
		} else {
			removeIndex = index;
			insertIndex -= 1;
		}

		object = objects[removeIndex];

		sqlite3_stmt *st = stmt[stmt_remove_playlist_by_range];
		if(sqlite3_reset(st) ||
		   sqlite3_bind_int64(st, remove_playlist_by_range_in_low, removeIndex) ||
		   sqlite3_bind_int64(st, remove_playlist_by_range_in_high, removeIndex) ||
		   sqlite3_step(st) != SQLITE_DONE)
			break;

		st = stmt[stmt_decrement_playlist_for_removal];
		if(sqlite3_reset(st) ||
		   sqlite3_bind_int64(st, decrement_playlist_for_removal_in_count, 1) ||
		   sqlite3_bind_int64(st, decrement_playlist_for_removal_in_index, removeIndex + 1) ||
		   sqlite3_step(st) != SQLITE_DONE)
			break;

		[databaseMirror removeObjectAtIndex:removeIndex];

		st = stmt[stmt_increment_playlist_for_insert];
		if(sqlite3_reset(st) ||
		   sqlite3_bind_int64(st, increment_playlist_for_insert_in_count, 1) ||
		   sqlite3_bind_int64(st, increment_playlist_for_insert_in_index, insertIndex) ||
		   sqlite3_step(st) != SQLITE_DONE)
			break;

		st = stmt[stmt_add_playlist];
		if(sqlite3_reset(st) ||
		   sqlite3_bind_int64(st, add_playlist_in_entry_index, insertIndex) ||
		   sqlite3_bind_int64(st, add_playlist_in_track_id, [object dbIndex]) ||
		   sqlite3_step(st) != SQLITE_DONE)
			break;

		[databaseMirror insertObject:object atIndex:insertIndex];

		index = [indexSet indexLessThanIndex:index];

		progress += progressstep;
		callback(progress);
	}

	callback(-1);
}

- (void)playlistMoveObjectsFromIndex:(NSUInteger)fromIndex
             toArrangedObjectIndexes:(NSIndexSet *)indexSet
                        progressCall:(void (^)(double))callback {
	__block NSUInteger rangeCount = 0;
	__block NSUInteger firstIndex = 0;
	__block NSUInteger _fromIndex = fromIndex;
	[indexSet enumerateRangesUsingBlock:^(NSRange range, BOOL *_Nonnull stop) {
		if(++rangeCount == 1)
			firstIndex = range.location;
		if(_fromIndex >= range.location) {
			if(_fromIndex < range.location + range.length)
				_fromIndex = range.location;
			else
				_fromIndex -= range.length;
		}
	}];

	if(rangeCount == 1 &&
	   (fromIndex >= firstIndex &&
	    fromIndex < firstIndex + [indexSet count])) // Null operation
		return;

	callback(0);
	double progress = 0;
	double progressstep = 50.0 / [indexSet count];

	fromIndex = _fromIndex;

	NSArray *objects = [databaseMirror subarrayWithRange:NSMakeRange(fromIndex, [indexSet count])];
	NSUInteger index = [indexSet firstIndex];

	NSUInteger itemIndex = 0;
	id object;

	sqlite3_stmt *st;

	fromIndex += [objects count];
	{
		st = stmt[stmt_remove_playlist_by_range];
		if(sqlite3_reset(st) ||
		   sqlite3_bind_int64(st, remove_playlist_by_range_in_low, fromIndex) ||
		   sqlite3_bind_int64(st, remove_playlist_by_range_in_high, fromIndex + [indexSet count] - 1) ||
		   sqlite3_step(st) != SQLITE_DONE) {
			callback(-1);
			return;
		}

		st = stmt[stmt_decrement_playlist_for_removal];
		if(sqlite3_reset(st) ||
		   sqlite3_bind_int64(st, decrement_playlist_for_removal_in_count, [indexSet count]) ||
		   sqlite3_bind_int64(st, decrement_playlist_for_removal_in_index, fromIndex) ||
		   sqlite3_step(st) != SQLITE_DONE) {
			callback(-1);
			return;
		}

		[databaseMirror removeObjectsInRange:NSMakeRange(fromIndex, [indexSet count])];

		progress += progressstep;
		callback(progress);
	}

	while(NSNotFound != index) {
		object = objects[itemIndex++];

		st = stmt[stmt_increment_playlist_for_insert];
		if(sqlite3_reset(st) ||
		   sqlite3_bind_int64(st, increment_playlist_for_insert_in_count, 1) ||
		   sqlite3_bind_int64(st, increment_playlist_for_insert_in_index, index) ||
		   sqlite3_step(st) != SQLITE_DONE)
			break;

		st = stmt[stmt_add_playlist];
		if(sqlite3_reset(st) ||
		   sqlite3_bind_int64(st, add_playlist_in_entry_index, index) ||
		   sqlite3_bind_int64(st, add_playlist_in_track_id, [object dbIndex]) ||
		   sqlite3_step(st) != SQLITE_DONE)
			break;

		[databaseMirror insertObject:object atIndex:index];

		index = [indexSet indexGreaterThanIndex:index];

		progress += progressstep;
		callback(progress);
	}

	callback(-1);
}

- (void)syncPlaylistEntries:(NSArray *)entries progressCall:(void (^)(double))callback {
	if(!entries || ![entries count]) {
		callback(-1);
		return;
	}

	int64_t count = [self playlistGetCountCached];

	if(count != [entries count]) {
		callback(-1);
		return;
	}

	callback(0);

	double progress = 0;
	double progressstep = 100.0 / (double)(count);

	sqlite3_stmt *st = stmt[stmt_update_playlist];

	for(size_t i = 0; i < count; ++i) {
		PlaylistEntry *newpe = [entries objectAtIndex:i];
		PlaylistEntry *oldpe = [databaseMirror objectAtIndex:i];

		progress += progressstep;

		if(([oldpe index] != i ||
		    [oldpe dbIndex] != [newpe dbIndex]) &&
		   [oldpe entryId] == [newpe entryId]) {
			if(sqlite3_reset(st) ||
			   sqlite3_bind_int64(st, update_playlist_in_id, [oldpe entryId]) ||
			   sqlite3_bind_int64(st, update_playlist_in_entry_index, i) ||
			   sqlite3_bind_int64(st, update_playlist_in_track_id, [newpe dbIndex]) ||
			   sqlite3_step(st) != SQLITE_ROW ||
			   sqlite3_reset(st)) {
				callback(-1);
				return;
			}

			[databaseMirror replaceObjectAtIndex:i withObject:newpe];

			callback(progress);
		}
	}

	sqlite3_reset(st);

	callback(-1);
}

- (void)queueAddItem:(int64_t)playlistIndex {
	int64_t count = [self queueGetCount];

	sqlite3_stmt *st = stmt[stmt_add_queue];

	if(sqlite3_reset(st) ||
	   sqlite3_bind_int64(st, add_queue_in_queue_index, count) ||
	   sqlite3_bind_int64(st, add_queue_in_entry_id, playlistIndex) ||
	   sqlite3_step(st) != SQLITE_DONE ||
	   sqlite3_reset(st)) {
		return;
	}
}

- (void)queueAddItems:(NSArray *)playlistIndexes {
	int64_t count = [self queueGetCount];

	sqlite3_stmt *st = stmt[stmt_add_queue];

	for(NSNumber *index in playlistIndexes) {
		if(sqlite3_reset(st) ||
		   sqlite3_bind_int64(st, add_queue_in_queue_index, count) ||
		   sqlite3_bind_int64(st, add_queue_in_entry_id, [index integerValue]) ||
		   sqlite3_step(st) != SQLITE_DONE) {
			break;
		}

		++count;
	}

	sqlite3_reset(st);
}

- (void)queueRemoveItem:(int64_t)queueIndex {
	sqlite3_stmt *st = stmt[stmt_remove_queue_by_index];

	if(sqlite3_reset(st) ||
	   sqlite3_bind_int64(st, remove_queue_by_index_in_queue_index, queueIndex) ||
	   sqlite3_step(st) != SQLITE_DONE ||
	   sqlite3_reset(st)) {
		return;
	}

	st = stmt[stmt_decrement_queue_for_removal];

	if(sqlite3_reset(st) ||
	   sqlite3_bind_int64(st, decrement_queue_for_removal_in_index, queueIndex + 1) ||
	   sqlite3_step(st) != SQLITE_DONE ||
	   sqlite3_reset(st)) {
		return;
	}
}

- (void)queueRemovePlaylistItems:(NSArray *)playlistIndexes {
	sqlite3_stmt *st = stmt[stmt_select_queue_by_playlist_entry];

	for(NSNumber *index in playlistIndexes) {
		if(sqlite3_reset(st) ||
		   sqlite3_bind_int64(st, select_queue_by_playlist_entry_in_id, [index integerValue]) ||
		   sqlite3_step(st) != SQLITE_ROW) {
			break;
		}

		int64_t queueIndex = sqlite3_column_int64(st, select_queue_by_playlist_entry_out_queue_index);

		sqlite3_reset(st);

		[self queueRemoveItem:queueIndex];
	}
}
#endif

- (int64_t)queueGetEntry:(int64_t)queueIndex {
	sqlite3_stmt *st = stmt[stmt_select_queue];

	if(sqlite3_reset(st) ||
	   sqlite3_bind_int64(st, select_queue_in_id, queueIndex)) {
		return -1;
	}

	int rc = sqlite3_step(st);

	if(rc != SQLITE_ROW && rc != SQLITE_DONE) {
		sqlite3_reset(st);
		return -1;
	}

	int64_t ret = -1;

	if(rc == SQLITE_ROW) {
		ret = sqlite3_column_int64(st, select_queue_out_entry_id);
	}

	sqlite3_reset(st);

	return ret;
}

#if 0
- (void)queueEmpty {
	sqlite3_stmt *st = stmt[stmt_remove_queue_all];

	if(sqlite3_reset(st) ||
	   sqlite3_step(st) != SQLITE_DONE ||
	   sqlite3_reset(st)) {
		return;
	}
}
#endif

- (int64_t)queueGetCount {
	sqlite3_stmt *st = stmt[stmt_count_queue];

	if(sqlite3_reset(st) ||
	   sqlite3_step(st) != SQLITE_ROW) {
		return 0;
	}

	int64_t ret = sqlite3_column_int64(st, count_queue_out_count);

	sqlite3_reset(st);

	return ret;
}

@end
