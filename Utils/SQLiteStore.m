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

@interface SQLiteStore (Private)
- (NSString *_Nullable)addString:(id _Nullable)string returnId:(int64_t *_Nonnull)stringId;
- (NSString *_Nonnull)getString:(int64_t)stringId;
- (void)removeString:(int64_t)stringId;
- (NSData *_Nullable)addArt:(id _Nullable)art returnId:(int64_t *_Nonnull)artId;
- (NSData *_Nonnull)getArt:(int64_t)artId;
- (void)removeArt:(int64_t)artId;
- (PlaylistEntry *_Nonnull)getTrack:(int64_t)trackId;
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

			entry.url = [PlaylistEntry urlForPath:[self getString:urlId]];

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
