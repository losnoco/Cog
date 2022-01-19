//
//  SQLiteStore.m
//  Cog
//
//  Created by Christopher Snowhill on 12/22/21.
//

#import <Foundation/Foundation.h>
#import "SQLiteStore.h"

NSString * getDatabasePath(void)
{
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
    NSString *basePath = [[paths firstObject] stringByAppendingPathComponent:@"Cog"];
    NSString *filename = @"Default.sqlite";
    return [basePath stringByAppendingPathComponent:filename];
}

NSArray * createSchema(void)
{
    return @[
@"CREATE TABLE IF NOT EXISTS stringdictionary ( \
        stringid INTEGER PRIMARY KEY AUTOINCREMENT, \
        referencecount INTEGER, \
        value TEXT NOT NULL \
    );",
@"CREATE TABLE IF NOT EXISTS artdictionary ( \
        artid INTEGER PRIMARY KEY AUTOINCREMENT, \
        referencecount INTEGER, \
        value BLOB NOT NULL \
    );",
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
        track INTEGER, \
        year INTEGER, \
        unsigned INTEGER, \
        bitrate INTEGER, \
        samplerate REAL, \
        bitspersample INTEGER, \
        channels INTEGER, \
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

enum
{
    stmt_select_string = 0,
    stmt_select_string_refcount,
    stmt_select_string_value,
    stmt_bump_string,
    stmt_pop_string,
    stmt_add_string,
    stmt_remove_string,
    
    stmt_select_art,
    stmt_select_art_refcount,
    stmt_select_art_value,
    stmt_bump_art,
    stmt_pop_art,
    stmt_add_art,
    stmt_remove_art,
    
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

enum
{
    select_string_in_id = 1,
    
    select_string_out_string_id = 0,
    select_string_out_reference_count,
};

const char * query_select_string = "SELECT stringid, referencecount FROM stringdictionary WHERE (value = ?) LIMIT 1";

enum
{
    select_string_refcount_in_id = 1,
    
    select_string_refcount_out_string_id = 0,
};

const char * query_select_string_refcount = "SELECT referencecount FROM stringdictionary WHERE (stringid = ?) LIMIT 1";

enum
{
    select_string_value_in_id = 1,
    
    select_string_value_out_value = 0,
};

const char * query_select_string_value = "SELECT value FROM stringdictionary WHERE (stringid = ?) LIMIT 1";

enum
{
    bump_string_in_id = 1,
};

const char * query_bump_string = "UPDATE stringdictionary SET referencecount = referencecount + 1 WHERE (stringid = ?) LIMIT 1";

enum
{
    pop_string_in_id = 1,
};

const char * query_pop_string = "UPDATE stringdictionary SET referencecount = referencecount - 1 WHERE (stringid = ?) LIMIT 1";

enum
{
    add_string_in_value = 1,
};

const char * query_add_string = "INSERT INTO stringdictionary (referencecount, value) VALUES (1, ?)";

enum
{
    remove_string_in_id = 1,
};

const char * query_remove_string = "DELETE FROM stringdictionary WHERE (stringid = ?)";


enum
{
    select_art_in_value = 1,
    
    select_art_out_art_id = 0,
    select_art_out_reference_count,
};

const char * query_select_art = "SELECT artid, referencecount FROM artdictionary WHERE (value = ?) LIMIT 1";

enum
{
    select_art_refcount_in_id = 1,
    
    select_art_refcount_out_reference_count = 0,
};

const char * query_select_art_refcount = "SELECT referencecount FROM artdictionary WHERE (artid = ?) LIMIT 1";

enum
{
    select_art_value_in_id = 1,
    
    select_art_value_out_value = 0,
};

const char * query_select_art_value = "SELECT value FROM artdictionary WHERE (artid = ?) LIMIT 1";

enum
{
    bump_art_in_id = 1,
};

const char * query_bump_art = "UPDATE artdictionary SET referencecount = referencecount + 1 WHERE (artid = ?) LIMIT 1";

enum
{
    pop_art_in_id = 1,
};

const char * query_pop_art = "UPDATE artdictionary SET referencecount = referencecount - 1 WHERE (artid = ?) LIMIT 1";

enum
{
    add_art_in_value = 1,
};

const char * query_add_art = "INSERT INTO artdictionary (referencecount, value) VALUES (1, ?)";

enum
{
    remove_art_in_id = 1,
};

const char * query_remove_art = "DELETE FROM artdictionary WHERE (artid = ?)";


enum
{
    select_track_in_id = 1,
    
    select_track_out_track_id = 0,
    select_track_out_reference_count,
};

const char * query_select_track = "SELECT trackid, referencecount FROM knowntracks WHERE (urlid = ?) LIMIT 1";

enum
{
    select_track_refcount_in_id = 1,
    
    select_track_refcount_out_reference_count = 0,
};

const char * query_select_track_refcount = "SELECT referencecount FROM knowntracks WHERE (trackid = ?) LIMIT 1";

enum
{
    select_track_data_in_id = 1,
    
    select_track_data_out_url_id = 0,
    select_track_data_out_art_id,
    select_track_data_out_album_id,
    select_track_data_out_albumartist_id,
    select_track_data_out_artist_id,
    select_track_data_out_title_id,
    select_track_data_out_genre_id,
    select_track_data_out_codec_id,
    select_track_data_out_track,
    select_track_data_out_year,
    select_track_data_out_unsigned,
    select_track_data_out_bitrate,
    select_track_data_out_samplerate,
    select_track_data_out_bitspersample,
    select_track_data_out_channels,
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

const char * query_select_track_data = "SELECT urlid, artid, albumid, albumartistid, artistid, titleid, genreid, codecid, track, year, unsigned, bitrate, samplerate, bitspersample, channels, endianid, floatingpoint, totalframes, metadataloaded, seekable, volume, replaygainalbumgain, replaygainalbumpeak, replaygaintrackgain, replaygaintrackpeak FROM knowntracks WHERE (trackid = ?) LIMIT 1";

enum
{
    bump_track_in_id = 1,
};

const char * query_bump_track = "UPDATE knowntracks SET referencecount = referencecount + 1 WHERE (trackid = ?) LIMIT 1";

enum
{
    pop_track_in_id = 1,
};

const char * query_pop_track = "UPDATE knowntracks SET referencecount = referencecount - 1 WHERE (trackid = ?) LIMIT 1";

enum
{
    add_track_in_url_id = 1,
    add_track_in_art_id,
    add_track_in_album_id,
    add_track_in_albumartist_id,
    add_track_in_artist_id,
    add_track_in_title_id,
    add_track_in_genre_id,
    add_track_in_codec_id,
    add_track_in_track,
    add_track_in_year,
    add_track_in_unsigned,
    add_track_in_bitrate,
    add_track_in_samplerate,
    add_track_in_bitspersample,
    add_track_in_channels,
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

const char * query_add_track = "INSERT INTO knowntracks (referencecount, urlid, artid, albumid, albumartistid, artistid, titleid, genreid, codecid, track, year, unsigned, bitrate, samplerate, bitspersample, channels, endianid, floatingpoint, totalframes, metadataloaded, seekable, volume, replaygainalbumgain, replaygainalbumpeak, replaygaintrackgain, replaygaintrackpeak) VALUES (1, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

enum
{
    remove_track_in_id = 1,
};

const char * query_remove_track = "DELETE FROM knowntracks WHERE (trackid = ?)";

enum
{
    update_track_in_url_id = 1,
    update_track_in_art_id,
    update_track_in_album_id,
    update_track_in_albumartist_id,
    update_track_in_artist_id,
    update_track_in_title_id,
    update_track_in_genre_id,
    update_track_in_codec_id,
    update_track_in_track,
    update_track_in_year,
    update_track_in_unsigned,
    update_track_in_bitrate,
    update_track_in_samplerate,
    update_track_in_bitspersample,
    update_track_in_channels,
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

const char * query_update_track = "UPDATE knowntracks SET urlid = ?, artid = ?, albumid = ?, albumartistid = ?, artistid = ?, titleid = ?, genreid = ?, codecid = ?, track = ?, year = ?, unsigned = ?, bitrate = ?, samplerate = ?, bitspersample = ?, channels = ?, endianid = ?, floatingpoint = ?, totalframes = ?, metadataloaded = ?, seekable = ?, volume = ?, replaygainalbumgain = ?, replaygainalbumpeak = ?, replaygaintrackgain = ?, replaygaintrackpeak = ? WHERE trackid = ?";


enum
{
    select_playlist_in_id = 1,
    
    select_playlist_out_entry_id = 0,
    select_playlist_out_track_id,
};

const char * query_select_playlist = "SELECT entryid, trackid FROM playlist WHERE (entryindex = ?)";

enum
{
    select_playlist_range_in_id_low = 1,
    select_playlist_range_in_id_high,
    
    select_playlist_range_out_entry_id = 0,
    select_playlist_range_out_track_id,
};

const char * query_select_playlist_range = "SELECT entryid, trackid FROM playlist WHERE (entryindex BETWEEN ? AND ?) ORDER BY entryindex ASC";

enum
{
    select_playlist_all_out_entry_id = 0,
    select_playlist_all_out_entry_index,
    select_playlist_all_out_track_id,
};

const char * query_select_playlist_all = "SELECT entryid, entryindex, trackid FROM playlist ORDER BY entryindex ASC";

enum
{
    increment_playlist_for_insert_in_count = 1,
    increment_playlist_for_insert_in_index,
};

const char * query_increment_playlist_for_insert = "UPDATE playlist SET entryindex = entryindex + ? WHERE (entryindex >= ?)";

enum
{
    decrement_playlist_for_removal_in_count = 1,
    decrement_playlist_for_removal_in_index,
};

const char * query_decrement_playlist_for_removal = "UPDATE playlist SET entryindex = entryindex - ? WHERE (entryindex >= ?)";

enum
{
    add_playlist_in_entry_index = 1,
    add_playlist_in_track_id,
};

const char * query_add_playlist = "INSERT INTO playlist(entryindex, trackid) VALUES (?, ?)";

enum
{
    remove_playlist_by_range_in_low = 1,
    remove_playlist_by_range_in_high,
};

const char * query_remove_playlist_by_range = "DELETE FROM playlist WHERE (entryindex BETWEEN ? AND ?)";

enum
{
    count_playlist_out_count = 0,
};

const char * query_count_playlist = "SELECT COUNT(*) FROM playlist";

enum
{
    update_playlist_in_entry_index = 1,
    update_playlist_in_track_id,
    update_playlist_in_id
};

const char * query_update_playlist = "UPDATE playlist SET entryindex = ?, trackid = ? WHERE (entryid = ?)";


enum
{
    select_queue_in_id = 1,
    
    select_queue_out_queue_id = 0,
    select_queue_out_entry_id,
};

const char * query_select_queue = "SELECT queueid, entryid FROM queue WHERE (queueindex = ?) LIMIT 1";

enum
{
    select_queue_by_playlist_entry_in_id = 1,
    
    select_queue_by_playlist_entry_out_queue_index = 0,
};

const char * query_select_queue_by_playlist_entry = "SELECT queueindex FROM queue WHERE (entryid = ?) LIMIT 1";

enum
{
    decrement_queue_for_removal_in_index = 1,
};

const char * query_decrement_queue_for_removal = "UPDATE queue SET queueindex = queueindex - 1 WHERE (queueindex >= ?)";

enum
{
    add_queue_in_queue_index = 1,
    add_queue_in_entry_id,
};

const char * query_add_queue = "INSERT INTO queue(queueindex, entryid) VALUES (?, ?)";

enum
{
    remove_queue_by_index_in_queue_index = 1,
};

const char * query_remove_queue_by_index = "DELETE FROM queue WHERE (queueindex = ?)";

const char * query_remove_queue_all = "DELETE FROM queue";

enum
{
    count_queue_out_count = 0,
};

const char * query_count_queue = "SELECT COUNT(*) FROM queue";


NSURL * urlForPath(NSString *path)
{
    if (!path || ![path length])
    {
        return [NSURL URLWithString:@"silence://10"];
    }
    
    NSRange protocolRange = [path rangeOfString:@"://"];
    if (protocolRange.location != NSNotFound) {
        return [NSURL URLWithString:path];
    }

    NSMutableString *unixPath = [path mutableCopy];

    //Get the fragment
    NSString *fragment = @"";
    NSScanner *scanner = [NSScanner scannerWithString:unixPath];
    NSCharacterSet *characterSet = [NSCharacterSet characterSetWithCharactersInString:@"#1234567890"];
    while (![scanner isAtEnd]) {
        NSString *possibleFragment;
        [scanner scanUpToString:@"#" intoString:nil];

        if ([scanner scanCharactersFromSet:characterSet intoString:&possibleFragment] && [scanner isAtEnd]) {
            fragment = possibleFragment;
            [unixPath deleteCharactersInRange:NSMakeRange([scanner scanLocation] - [possibleFragment length], [possibleFragment length])];
            break;
        }
    }

    //Append the fragment
    NSURL *url = [NSURL URLWithString:[[[NSURL fileURLWithPath:unixPath] absoluteString] stringByAppendingString:fragment]];
    return url;
}

@interface SQLiteStore (Private)
- (int64_t) addString:(NSString *)string;
- (NSString *) getString:(int64_t)stringId;
- (void) removeString:(int64_t)stringId;
- (int64_t) addArt:(NSData *)art;
- (NSData *) getArt:(int64_t)artId;
- (void) removeArt:(int64_t)artId;
- (int64_t) addTrack:(PlaylistEntry *)track;
- (PlaylistEntry *) getTrack:(int64_t)trackId;
- (void) removeTrack:(int64_t)trackId;
@end

@implementation SQLiteStore

static SQLiteStore *g_sharedStore = NULL;

+ (SQLiteStore *)sharedStore
{
    if (!g_sharedStore)
    {
        g_sharedStore = [[SQLiteStore alloc] init];
    }
    
    return g_sharedStore;
}

@synthesize databasePath = g_databasePath;
@synthesize database = g_database;

- (id) init;
{
    self = [super init];

    if (self)
    {
        g_databasePath = getDatabasePath();
        
        memset(stmt, 0, sizeof(stmt));
        
        if (sqlite3_open([g_databasePath UTF8String], &g_database) == SQLITE_OK)
        {
            NSArray * schemas = createSchema();
            
            for (NSString *schema in schemas)
            {
                char * error;
                if (sqlite3_exec(g_database, [schema UTF8String], NULL, NULL, &error) != SQLITE_OK)
                {
                    return nil;
                }
            }

#define PREPARE(name) (sqlite3_prepare(g_database, query_##name, (int)strlen(query_##name), &stmt[stmt_##name], NULL))

            if (PREPARE(select_string) ||
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
                PREPARE(count_queue))
            {
                return nil;
            }
#undef PREPARE
            
            size_t count = [self playlistGetCount];
            
            databaseMirror = [[NSMutableArray alloc] init];
            
            for (size_t i = 0; i < count; ++i)
            {
                PlaylistEntry *pe = [self playlistGetItem:i];
                [databaseMirror addObject:pe];
            }
            
            return self;
        }
    }
    
    return nil;
}

- (void) dealloc
{
    if (g_database)
    {
        for (size_t i = 0; i < stmt_count; ++i)
        {
            if (stmt[i]) sqlite3_finalize(stmt[i]);
        }
        sqlite3_close(g_database);
    }
}

- (int64_t) addString:(NSString *)string
{
    if (!string || [string length] == 0)
    {
        return -1;
    }
    
    const char * str = [string UTF8String];
    uint64_t len = strlen(str); // SQLite expects number of bytes, not characters
    
    sqlite3_stmt *st = stmt[stmt_select_string];
    
    if (sqlite3_reset(st) ||
        sqlite3_bind_text64(st, select_string_in_id, str, len, SQLITE_STATIC, SQLITE_UTF8))
    {
        return -1;
    }
    
    int64_t ret, refcount;
    
    int rc = sqlite3_step(st);
    
    if (rc != SQLITE_DONE && rc != SQLITE_ROW)
    {
        return -1;
    }
    
    if (rc == SQLITE_ROW)
    {
        ret = sqlite3_column_int64(st, select_string_out_string_id);
        refcount = sqlite3_column_int64(st, select_string_out_reference_count);
    }
    
    sqlite3_reset(st);
    
    if (rc != SQLITE_ROW)
    {
        st = stmt[stmt_add_string];
        
        if (sqlite3_reset(st) ||
            sqlite3_bind_text64(st, add_string_in_value, str, len, SQLITE_STATIC, SQLITE_UTF8) ||
            sqlite3_step(st) != SQLITE_DONE ||
            sqlite3_reset(st))
        {
            return -1;
        }
        
        ret = sqlite3_last_insert_rowid(g_database);
        refcount = 1;
    }
    else
    {
        st = stmt[stmt_bump_string];
        
        if (sqlite3_reset(st) ||
            sqlite3_bind_int64(st, bump_string_in_id, ret) ||
            sqlite3_step(st) != SQLITE_DONE ||
            sqlite3_reset(st))
        {
            return -1;
        }
    }
    
    return ret;
}

- (NSString *) getString:(int64_t)stringId
{
    if (stringId < 0)
        return @"";
    
    sqlite3_stmt *st = stmt[stmt_select_string_value];
    
    if (sqlite3_reset(st) ||
        sqlite3_bind_int64(st, select_string_value_in_id, stringId))
    {
        return @"";
    }
    
    int rc = sqlite3_step(st);
    
    if (rc != SQLITE_ROW && rc != SQLITE_DONE)
    {
        sqlite3_reset(st);
        return @"";
    }
    
    NSString *ret = @"";
    
    if (rc == SQLITE_ROW)
    {
        const unsigned char *str = sqlite3_column_text(st, select_string_value_out_value);
        int strBytes = sqlite3_column_bytes(st, select_string_value_out_value);
        if (str && strBytes && *str)
        {
            ret = [NSString stringWithUTF8String:(const char *)str];
        }
    }
    
    sqlite3_reset(st);
    
    return ret;
}

- (void) removeString:(int64_t)stringId
{
    if (stringId < 0)
        return;
    
    int64_t refcount;
    
    sqlite3_stmt *st = stmt[stmt_select_string_refcount];
    
    if (sqlite3_reset(st) ||
        sqlite3_bind_int64(st, select_string_refcount_in_id, stringId))
    {
        return;
    }
    
    int rc = sqlite3_step(st);
    
    if (rc != SQLITE_ROW && rc != SQLITE_DONE)
    {
        return;
    }
    
    if (rc == SQLITE_ROW)
    {
        refcount = sqlite3_column_int64(st, select_string_refcount_out_string_id);
    }
    
    sqlite3_reset(st);
    
    if (rc != SQLITE_ROW)
    {
        refcount = 1;
    }
    
    if (refcount <= 1)
    {
        st = stmt[stmt_remove_string];
        
        if (sqlite3_reset(st) ||
            sqlite3_bind_int64(st, remove_string_in_id, stringId) ||
            sqlite3_step(st) != SQLITE_DONE ||
            sqlite3_reset(st))
        {
            return;
        }
    }
    else
    {
        st = stmt[stmt_pop_string];
        
        if (sqlite3_reset(st) ||
            sqlite3_bind_int64(st, pop_string_in_id, stringId) ||
            sqlite3_step(st) != SQLITE_DONE ||
            sqlite3_reset(st))
        {
            return;
        }
    }
}

- (int64_t) addArt:(NSData *)art
{
    if (!art || [art length] == 0)
    {
        return -1;
    }
    
    sqlite3_stmt *st = stmt[stmt_select_art];
    
    if (sqlite3_reset(st) ||
        sqlite3_bind_blob64(st, select_art_in_value, [art bytes], [art length], SQLITE_STATIC))
    {
        return -1;
    }
    
    int64_t ret, refcount;
    
    int rc = sqlite3_step(st);
    
    if (rc != SQLITE_DONE && rc != SQLITE_ROW)
    {
        return -1;
    }
    
    if (rc == SQLITE_ROW)
    {
        ret = sqlite3_column_int64(st, select_art_out_art_id);
        refcount = sqlite3_column_int64(st, select_art_out_reference_count);
    }
    
    sqlite3_reset(st);
    
    if (rc != SQLITE_ROW)
    {
        st = stmt[stmt_add_art];
        
        if (sqlite3_reset(st) ||
            sqlite3_bind_blob64(st, add_art_in_value, [art bytes], [art length], SQLITE_STATIC) ||
            sqlite3_step(st) != SQLITE_DONE ||
            sqlite3_reset(st))
        {
            return -1;
        }
        
        ret = sqlite3_last_insert_rowid(g_database);
        refcount = 1;
    }
    else
    {
        st = stmt[stmt_bump_art];
        
        if (sqlite3_reset(st) ||
            sqlite3_bind_int64(st, bump_art_in_id, ret) ||
            sqlite3_step(st) != SQLITE_DONE ||
            sqlite3_reset(st))
        {
            return -1;
        }
    }
    
    return ret;
}

- (NSData *) getArt:(int64_t)artId
{
    if (artId < 0)
        return [NSData data];
    
    sqlite3_stmt *st = stmt[stmt_select_art_value];
    
    if (sqlite3_reset(st) ||
        sqlite3_bind_int64(st, select_art_value_in_id, artId))
    {
        return [NSData data];
    }
    
    int rc = sqlite3_step(st);
    
    if (rc != SQLITE_ROW && rc != SQLITE_DONE)
    {
        sqlite3_reset(st);
        return [NSData data];
    }
    
    NSData *ret = [NSData data];
    
    if (rc == SQLITE_ROW)
    {
        const void * blob = sqlite3_column_blob(st, select_art_value_out_value);
        int blobBytes = sqlite3_column_bytes(st, select_art_value_out_value);
        if (blob && blobBytes)
        {
            ret = [NSData dataWithBytes:blob length:blobBytes];
        }
    }
    
    sqlite3_reset(st);
    
    return ret;
}

- (void) removeArt:(int64_t)artId
{
    if (artId < 0)
        return;
    
    int64_t refcount;
    
    sqlite3_stmt *st = stmt[stmt_select_art_refcount];
    
    if (sqlite3_reset(st) ||
        sqlite3_bind_int64(st, select_art_refcount_in_id, artId))
    {
        return;
    }
    
    int rc = sqlite3_step(st);
    
    if (rc != SQLITE_ROW && rc != SQLITE_DONE)
    {
        return;
    }
    
    if (rc == SQLITE_ROW)
    {
        refcount = sqlite3_column_int64(st, select_art_refcount_out_reference_count);
    }
    
    sqlite3_reset(st);
    
    if (rc != SQLITE_ROW)
    {
        refcount = 1;
    }
    
    if (refcount <= 1)
    {
        st = stmt[stmt_remove_art];
        
        if (sqlite3_reset(st) ||
            sqlite3_bind_int64(st, remove_art_in_id, artId) ||
            sqlite3_step(st) != SQLITE_DONE ||
            sqlite3_reset(st))
        {
            return;
        }
    }
    else
    {
        st = stmt[stmt_pop_art];
        
        if (sqlite3_reset(st) ||
            sqlite3_bind_int64(st, pop_art_in_id, artId) ||
            sqlite3_step(st) != SQLITE_DONE ||
            sqlite3_reset(st))
        {
            return;
        }
    }
}

- (int64_t) addTrack:(PlaylistEntry *)track
{
    NSURL *url = [track URL];
    NSString *urlString = [url absoluteString];
    
    int64_t urlId = [self addString:urlString];
    
    sqlite3_stmt *st = stmt[stmt_select_track];
    
    if (sqlite3_reset(st) ||
        sqlite3_bind_int64(st, select_track_in_id, urlId))
    {
        [self removeString:urlId];
        return -1;
    }
    
    int64_t ret, refcount;
    
    int rc = sqlite3_step(st);
    
    if (rc != SQLITE_DONE && rc != SQLITE_ROW)
    {
        [self removeString:urlId];
        return -1;
    }
    
    if (rc == SQLITE_ROW)
    {
        ret = sqlite3_column_int64(st, select_track_out_track_id);
        refcount = sqlite3_column_int64(st, select_track_out_reference_count);
    }
    
    sqlite3_reset(stmt[stmt_select_string]);

    if (rc != SQLITE_ROW)
    {
        int64_t albumId = [self addString:[track album]];
        int64_t albumartistId = [self addString:[track albumartist]];
        int64_t artistId = [self addString:[track artist]];
        int64_t titleId = [self addString:[track title]];
        int64_t genreId = [self addString:[track genre]];
        int64_t codecId = [self addString:[track codec]];
        int64_t trackNr = [[track track] intValue];
        int64_t year = [[track year] intValue];
        int64_t unsignedFmt = [track Unsigned];
        int64_t bitrate = [track bitrate];
        double samplerate = [track sampleRate];
        int64_t bitspersample = [track bitsPerSample];
        int64_t channels = [track channels];
        int64_t endianId = [self addString:[track endian]];
        int64_t floatingpoint = [track floatingPoint];
        int64_t totalframes = [track totalFrames];
        int64_t metadataloaded = [track metadataLoaded];
        int64_t seekable = [track seekable];
        double volume = [track volume];
        double replaygainalbumgain = [track replayGainAlbumGain];
        double replaygainalbumpeak = [track replayGainAlbumPeak];
        double replaygaintrackgain = [track replayGainTrackGain];
        double replaygaintrackpeak = [track replayGainTrackPeak];
        
        NSData *albumArt = [track albumArtInternal];
        int64_t artId = -1;
        
        if (albumArt)
            artId = [self addArt:albumArt];
        
        st = stmt[stmt_add_track];
        
        if (sqlite3_reset(st) ||
            sqlite3_bind_int64(st, add_track_in_url_id, urlId) ||
            sqlite3_bind_int64(st, add_track_in_art_id, artId) ||
            sqlite3_bind_int64(st, add_track_in_album_id, albumId) ||
            sqlite3_bind_int64(st, add_track_in_albumartist_id, albumartistId) ||
            sqlite3_bind_int64(st, add_track_in_artist_id, artistId) ||
            sqlite3_bind_int64(st, add_track_in_title_id, titleId) ||
            sqlite3_bind_int64(st, add_track_in_genre_id, genreId) ||
            sqlite3_bind_int64(st, add_track_in_codec_id, codecId) ||
            sqlite3_bind_int64(st, add_track_in_track, trackNr) ||
            sqlite3_bind_int64(st, add_track_in_year, year) ||
            sqlite3_bind_int64(st, add_track_in_unsigned, unsignedFmt) ||
            sqlite3_bind_int64(st, add_track_in_bitrate, bitrate) ||
            sqlite3_bind_double(st, add_track_in_samplerate, samplerate) ||
            sqlite3_bind_int64(st, add_track_in_bitspersample, bitspersample) ||
            sqlite3_bind_int64(st, add_track_in_channels, channels) ||
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
            sqlite3_reset(st))
        {
            [self removeArt:artId];
            
            [self removeString:albumId];
            [self removeString:albumartistId];
            [self removeString:artistId];
            [self removeString:titleId];
            [self removeString:genreId];
            [self removeString:codecId];
            [self removeString:endianId];
            
            [self removeString:urlId];

            return -1;
        }

        ret = sqlite3_last_insert_rowid(g_database);
        refcount = 1;
    }
    else
    {
        [self removeString:urlId]; // should only be bumped once per instance of track
        
        st = stmt[stmt_bump_track];
        
        if (sqlite3_reset(st) ||
            sqlite3_bind_int64(st, bump_track_in_id, ret) ||
            sqlite3_step(st) != SQLITE_DONE ||
            sqlite3_reset(st))
        {
            return -1;
        }
    }
    
    [track setDbIndex:ret];
    
    return ret;
}

- (void) trackUpdate:(PlaylistEntry *)track
{
    NSURL *url = [track URL];
    NSString *urlString = [url absoluteString];
    
    int64_t urlId = [self addString:urlString];
    
    sqlite3_stmt *st = stmt[stmt_select_track];
    
    if (sqlite3_reset(st) ||
        sqlite3_bind_int64(st, select_track_in_id, urlId))
    {
        [self removeString:urlId];
        return;
    }
    
    int64_t trackId, refcount;
    
    int rc = sqlite3_step(st);
    
    if (rc != SQLITE_DONE && rc != SQLITE_ROW)
    {
        [self removeString:urlId];
        return;
    }
    
    trackId = -1;
    
    if (rc == SQLITE_ROW)
    {
        trackId = sqlite3_column_int64(st, select_track_out_track_id);
        refcount = sqlite3_column_int64(st, select_track_out_reference_count);
    }
    
    sqlite3_reset(stmt[stmt_select_string]);
    
    if (trackId < 0)
    {
        [self removeString:urlId];
        return;
    }
    
    if (trackId != [track dbIndex])
    {
        [self removeString:urlId];
        return;
    }

    st = stmt[stmt_select_track_data];
    
    if (sqlite3_reset(st) ||
        sqlite3_bind_int64(st, select_track_data_in_id, trackId))
    {
        [self removeString:urlId];
        return;
    }
    
    rc = sqlite3_step(st);
    
    if (rc != SQLITE_ROW && rc != SQLITE_DONE)
    {
        sqlite3_reset(st);
        [self removeString:urlId];
        return;
    }
    
    if (rc == SQLITE_ROW)
    {
        int64_t urlId = sqlite3_column_int64(st, select_track_data_out_url_id);
        int64_t artId = sqlite3_column_int64(st, select_track_data_out_art_id);
        int64_t albumId = sqlite3_column_int64(st, select_track_data_out_album_id);
        int64_t albumartistId = sqlite3_column_int64(st, select_track_data_out_albumartist_id);
        int64_t artistId = sqlite3_column_int64(st, select_track_data_out_artist_id);
        int64_t titleId = sqlite3_column_int64(st, select_track_data_out_title_id);
        int64_t genreId = sqlite3_column_int64(st, select_track_data_out_genre_id);
        int64_t codecId = sqlite3_column_int64(st, select_track_data_out_codec_id);
        int64_t endianId = sqlite3_column_int64(st, select_track_data_out_endian_id);
        
        [self removeArt:artId];

        [self removeString:urlId];
        [self removeString:albumId];
        [self removeString:albumartistId];
        [self removeString:artistId];
        [self removeString:titleId];
        [self removeString:genreId];
        [self removeString:codecId];
        [self removeString:endianId];
    }
    
    sqlite3_reset(st);

    {
        int64_t albumId = [self addString:[track album]];
        int64_t albumartistId = [self addString:[track albumartist]];
        int64_t artistId = [self addString:[track artist]];
        int64_t titleId = [self addString:[track title]];
        int64_t genreId = [self addString:[track genre]];
        int64_t codecId = [self addString:[track codec]];
        int64_t trackNr = [[track track] intValue];
        int64_t year = [[track year] intValue];
        int64_t unsignedFmt = [track Unsigned];
        int64_t bitrate = [track bitrate];
        double samplerate = [track sampleRate];
        int64_t bitspersample = [track bitsPerSample];
        int64_t channels = [track channels];
        int64_t endianId = [self addString:[track endian]];
        int64_t floatingpoint = [track floatingPoint];
        int64_t totalframes = [track totalFrames];
        int64_t metadataloaded = [track metadataLoaded];
        int64_t seekable = [track seekable];
        double volume = [track volume];
        double replaygainalbumgain = [track replayGainAlbumGain];
        double replaygainalbumpeak = [track replayGainAlbumPeak];
        double replaygaintrackgain = [track replayGainTrackGain];
        double replaygaintrackpeak = [track replayGainTrackPeak];
        
        NSData *albumArt = [track albumArtInternal];
        int64_t artId = -1;
        
        if (albumArt)
            artId = [self addArt:albumArt];
        
        st = stmt[stmt_update_track];
        
        if (sqlite3_reset(st) ||
            sqlite3_bind_int64(st, update_track_in_url_id, urlId) ||
            sqlite3_bind_int64(st, update_track_in_art_id, artId) ||
            sqlite3_bind_int64(st, update_track_in_album_id, albumId) ||
            sqlite3_bind_int64(st, update_track_in_albumartist_id, albumartistId) ||
            sqlite3_bind_int64(st, update_track_in_artist_id, artistId) ||
            sqlite3_bind_int64(st, update_track_in_title_id, titleId) ||
            sqlite3_bind_int64(st, update_track_in_genre_id, genreId) ||
            sqlite3_bind_int64(st, update_track_in_codec_id, codecId) ||
            sqlite3_bind_int64(st, update_track_in_track, trackNr) ||
            sqlite3_bind_int64(st, update_track_in_year, year) ||
            sqlite3_bind_int64(st, update_track_in_unsigned, unsignedFmt) ||
            sqlite3_bind_int64(st, update_track_in_bitrate, bitrate) ||
            sqlite3_bind_double(st, update_track_in_samplerate, samplerate) ||
            sqlite3_bind_int64(st, update_track_in_bitspersample, bitspersample) ||
            sqlite3_bind_int64(st, update_track_in_channels, channels) ||
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
            sqlite3_reset(st))
        {
            [self removeArt:artId];
            
            [self removeString:albumId];
            [self removeString:albumartistId];
            [self removeString:artistId];
            [self removeString:titleId];
            [self removeString:genreId];
            [self removeString:codecId];
            [self removeString:endianId];
            
            [self removeString:urlId];

            return;
        }

        [databaseMirror replaceObjectAtIndex:[track index] withObject:track];
    }
}

- (PlaylistEntry *) getTrack:(int64_t)trackId
{
    PlaylistEntry *entry = [[PlaylistEntry alloc] init];
    
    if (trackId < 0)
        return entry;
    
    sqlite3_stmt *st = stmt[stmt_select_track_data];
    
    if (sqlite3_reset(st) ||
        sqlite3_bind_int64(st, select_track_data_in_id, trackId))
    {
        return entry;
    }
    
    int rc = sqlite3_step(st);
    
    if (rc != SQLITE_ROW && rc != SQLITE_DONE)
    {
        sqlite3_reset(st);
        return entry;
    }
    
    if (rc == SQLITE_ROW)
    {
        int64_t urlId = sqlite3_column_int64(st, select_track_data_out_url_id);
        int64_t artId = sqlite3_column_int64(st, select_track_data_out_art_id);
        int64_t albumId = sqlite3_column_int64(st, select_track_data_out_album_id);
        int64_t albumartistId = sqlite3_column_int64(st, select_track_data_out_albumartist_id);
        int64_t artistId = sqlite3_column_int64(st, select_track_data_out_artist_id);
        int64_t titleId = sqlite3_column_int64(st, select_track_data_out_title_id);
        int64_t genreId = sqlite3_column_int64(st, select_track_data_out_genre_id);
        int64_t codecId = sqlite3_column_int64(st, select_track_data_out_codec_id);
        int64_t trackNr = sqlite3_column_int64(st, select_track_data_out_track);
        int64_t year = sqlite3_column_int64(st, select_track_data_out_year);
        int64_t unsignedFmt = sqlite3_column_int64(st, select_track_data_out_unsigned);
        int64_t bitrate = sqlite3_column_int64(st, select_track_data_out_bitrate);
        double samplerate = sqlite3_column_double(st, select_track_data_out_samplerate);
        int64_t bitspersample = sqlite3_column_int64(st, select_track_data_out_bitspersample);
        int64_t channels = sqlite3_column_int64(st, select_track_data_out_channels);
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

        [entry setURL:urlForPath([self getString:urlId])];
        
        [entry setAlbum:[self getString:albumId]];
        [entry setAlbumartist:[self getString:albumartistId]];
        [entry setArtist:[self getString:artistId]];
        [entry setTitle:[self getString:titleId]];
        [entry setGenre:[self getString:genreId]];
        [entry setCodec:[self getString:codecId]];
        [entry setTrack:[NSNumber numberWithInteger:trackNr]];
        [entry setYear:[NSNumber numberWithInteger:year]];
        [entry setUnsigned:!!unsignedFmt];
        [entry setBitrate:(int)bitrate];
        [entry setSampleRate:samplerate];
        [entry setBitsPerSample:(int)bitspersample];
        [entry setChannels:(int)channels];
        [entry setEndian:[self getString:endianId]];
        [entry setFloatingPoint:!!floatingpoint];
        [entry setTotalFrames:totalframes];
        [entry setSeekable:!!seekable];
        [entry setVolume:volume];
        [entry setReplayGainAlbumGain:replaygainalbumgain];
        [entry setReplayGainAlbumPeak:replaygainalbumpeak];
        [entry setReplayGainTrackGain:replaygaintrackgain];
        [entry setReplayGainTrackPeak:replaygaintrackpeak];
        
        [entry setAlbumArtInternal:[self getArt:artId]];
        
        [entry setMetadataLoaded:!!metadataloaded];
        
        [entry setDbIndex:trackId];
    }
    
    sqlite3_reset(st);
    
    return entry;
}

- (void) removeTrack:(int64_t)trackId
{
    if (trackId < 0)
        return;
    
    int64_t refcount;
    
    sqlite3_stmt *st = stmt[stmt_select_track_refcount];
    
    if (sqlite3_reset(st) ||
        sqlite3_bind_int64(st, select_track_refcount_in_id, trackId))
    {
        return;
    }
    
    int rc = sqlite3_step(st);
    
    if (rc != SQLITE_ROW && rc != SQLITE_DONE)
    {
        return;
    }
    
    if (rc == SQLITE_ROW)
    {
        refcount = sqlite3_column_int64(st, select_track_refcount_out_reference_count);
    }
    
    sqlite3_reset(st);
    
    if (rc != SQLITE_ROW)
    {
        refcount = 1;
    }
    
    if (refcount <= 1)
    {
        // DeRef the strings and art
        
        st = stmt[stmt_select_track_data];
        
        if (sqlite3_reset(st) == SQLITE_OK &&
            sqlite3_bind_int64(st, select_track_data_in_id, trackId) == SQLITE_OK &&
            sqlite3_step(st) == SQLITE_ROW)
        {
            int64_t urlId = sqlite3_column_int64(st, select_track_data_out_url_id);
            int64_t artId = sqlite3_column_int64(st, select_track_data_out_art_id);
            int64_t albumId = sqlite3_column_int64(st, select_track_data_out_album_id);
            int64_t albumartistId = sqlite3_column_int64(st, select_track_data_out_albumartist_id);
            int64_t artistId = sqlite3_column_int64(st, select_track_data_out_artist_id);
            int64_t titleId = sqlite3_column_int64(st, select_track_data_out_title_id);
            int64_t genreId = sqlite3_column_int64(st, select_track_data_out_genre_id);
            int64_t codecId = sqlite3_column_int64(st, select_track_data_out_codec_id);
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
            [self removeString:endianId];
        }
        
        st = stmt[stmt_remove_track];
        
        if (sqlite3_reset(st) ||
            sqlite3_bind_int64(st, remove_track_in_id, trackId) ||
            sqlite3_step(st) != SQLITE_DONE ||
            sqlite3_reset(st))
        {
            return;
        }
    }
    else
    {
        st = stmt[stmt_pop_track];
        
        if (sqlite3_reset(st) ||
            sqlite3_bind_int64(st, pop_track_in_id, trackId) ||
            sqlite3_step(st) != SQLITE_DONE ||
            sqlite3_reset(st))
        {
            return;
        }
    }
}

- (void)playlistInsertTracks:(NSArray *)tracks atIndex:(int64_t)index progressCall:(void (^)(double))callback
{
    if (!tracks)
    {
        callback(-1);
        return;
    }
    
    callback(0);
    
    sqlite3_stmt *st = stmt[stmt_increment_playlist_for_insert];
    
    if (sqlite3_reset(st) ||
        sqlite3_bind_int64(st, increment_playlist_for_insert_in_count, [tracks count]) ||
        sqlite3_bind_int64(st, increment_playlist_for_insert_in_index, index) ||
        sqlite3_step(st) != SQLITE_DONE ||
        sqlite3_reset(st))
    {
        callback(-1);
        return;
    }
    
    callback(25);
    
    double progress = 25.0;
    double progressstep = [tracks count] ? 75.0 / (double)([tracks count]) : 0;
    
    st = stmt[stmt_add_playlist];
    
    for (PlaylistEntry *entry in tracks)
    {
        int64_t trackId = [self addTrack:entry];
        
        if (sqlite3_reset(st) ||
            sqlite3_bind_int64(st, add_playlist_in_entry_index, index) ||
            sqlite3_bind_int64(st, add_playlist_in_track_id, trackId) ||
            sqlite3_step(st) != SQLITE_DONE)
        {
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

- (void)playlistInsertTracks:(NSArray *)tracks atObjectIndexes:(NSIndexSet *)indexes progressCall:(void (^)(double))callback
{
    if (!tracks || !indexes)
    {
        callback(-1);
        return;
    }
    
    [databaseMirror insertObjects:tracks atIndexes:indexes];
    
    __block int64_t total_count = 0;
    [indexes enumerateRangesUsingBlock:^(NSRange range, BOOL * _Nonnull stop) {
        total_count += range.length;
    }];
    
    __block int64_t i = 0;
    
    __block double progress = 0;
    
    [indexes enumerateRangesUsingBlock:^(NSRange range, BOOL * _Nonnull stop) {
        double progresschunk = (double)range.length / (double)total_count;
        double progressbase = progress;
        NSRange trackRange = NSMakeRange(i, range.length);
        NSArray *trackSet = (i == 0 && range.length == [tracks count]) ? tracks : [tracks subarrayWithRange:trackRange];
        [self playlistInsertTracks:trackSet atIndex:range.location progressCall:^(double _progress){
            if (_progress < 0) return;
            callback(progressbase + progresschunk * _progress);
        }];
        i += range.length;
        progress += 100.0 * progresschunk;
        callback(progress);
    }];
    callback(-1);
}

- (void)playlistRemoveTracks:(int64_t)index forCount:(int64_t)count progressCall:(void (^)(double))callback
{
    if (!count)
    {
        callback(-1);
        return;
    }
    
    sqlite3_stmt *st = stmt[stmt_select_playlist_range];

    if (sqlite3_reset(st) ||
        sqlite3_bind_int64(st, select_playlist_range_in_id_low, index) ||
        sqlite3_bind_int64(st, select_playlist_range_in_id_high, index + count - 1))
    {
        callback(-1);
        return;
    }
    
    callback(0);
    
    double progress = 0;
    double progressstep = 100.0 / ((double)count);
    
    int rc = sqlite3_step(st);
    
    while (rc == SQLITE_ROW)
    {
        int64_t trackId = sqlite3_column_int64(st, select_playlist_range_out_track_id);
        [self removeTrack:trackId];
        rc = sqlite3_step(st);
        progress += progressstep;
        callback(progress);
    }
    
    callback(100);
    
    sqlite3_reset(st);
    
    if (rc != SQLITE_DONE)
    {
        return;
    }
    
    st = stmt[stmt_remove_playlist_by_range];
    
    if (sqlite3_reset(st) ||
        sqlite3_bind_int64(st, remove_playlist_by_range_in_low, index) ||
        sqlite3_bind_int64(st, remove_playlist_by_range_in_high, index + count - 1) ||
        sqlite3_step(st) != SQLITE_DONE ||
        sqlite3_reset(st))
    {
        callback(-1);
        return;
    }
    
    st = stmt[stmt_decrement_playlist_for_removal];
    
    if (sqlite3_reset(st) ||
        sqlite3_bind_int64(st, decrement_playlist_for_removal_in_count, count) ||
        sqlite3_bind_int64(st, decrement_playlist_for_removal_in_index, index + count) ||
        sqlite3_step(st) != SQLITE_DONE ||
        sqlite3_reset(st))
    {
        callback(-1);
        return;
    }

    NSMutableArray *items = [[NSMutableArray alloc] init];
    
    for (int64_t i = index, j = index + count; i < j; ++i)
    {
        [items addObject:[NSNumber numberWithInteger:i]];
    }
    
    [self queueRemovePlaylistItems:items];
    
    callback(-1);
}

- (void)playlistRemoveTracksAtIndexes:(NSIndexSet *)indexes progressCall:(void (^)(double))callback
{
    if (!indexes)
    {
        callback(-1);
        return;
    }
    
    [databaseMirror removeObjectsAtIndexes:indexes];
    
    __block int64_t total_count = 0;
    
    [indexes enumerateRangesUsingBlock:^(NSRange range, BOOL * _Nonnull stop) {
        total_count += range.length;
    }];
    
    __block int64_t i = 0;
    
    __block double progress = 0;
    
    callback(progress);
    
    [indexes enumerateRangesUsingBlock:^(NSRange range, BOOL * _Nonnull stop) {
        double progresschunk = (double)range.length / (double)total_count;
        double progressbase = progress;
        [self playlistRemoveTracks:(range.location - i) forCount:range.length progressCall:^(double _progress) {
            if (_progress < 0) return;
            callback(progressbase + progresschunk * _progress);
        }];
        i += range.length;
        progress += 100.0 * progresschunk;
        callback(progress);
    }];
    callback(-1);
}

- (PlaylistEntry *)playlistGetCachedItem:(int64_t)index
{
    if (index >= 0 && index < [databaseMirror count])
        return [databaseMirror objectAtIndex:index];
    else
        return nil;
}

- (PlaylistEntry *)playlistGetItem:(int64_t)index
{
    PlaylistEntry *entry = [[PlaylistEntry alloc] init];
    
    sqlite3_stmt *st = stmt[stmt_select_playlist];
    
    if (sqlite3_reset(st) ||
        sqlite3_bind_int64(st, select_playlist_in_id, index))
    {
        return entry;
    }
    
    int rc = sqlite3_step(st);
    
    if (rc != SQLITE_ROW && rc != SQLITE_DONE)
    {
        return entry;
    }
    
    if (rc == SQLITE_ROW)
    {
        int64_t trackId = sqlite3_column_int64(st, select_playlist_out_track_id);
        int64_t entryId = sqlite3_column_int64(st, select_playlist_out_entry_id);
        entry = [self getTrack:trackId];
        [entry setIndex:index];
        [entry setEntryId:entryId];
    }
    
    sqlite3_reset(st);
    
    return entry;
}

- (int64_t)playlistGetCount
{
    sqlite3_stmt *st = stmt[stmt_count_playlist];
    
    if (sqlite3_reset(st) ||
        sqlite3_step(st) != SQLITE_ROW)
    {
        return 0;
    }
    
    int64_t ret = sqlite3_column_int64(st, count_playlist_out_count);
    
    sqlite3_reset(st);
    
    return ret;
}

#if 0 // syncPlaylistEntries is already called where this would be needed
- (void)playlistMoveObjectsInArrangedObjectsFromIndexes:(NSIndexSet *)indexSet toIndex:(NSUInteger)insertIndex
{
    __block NSMutableArray *entryIds = [[NSMutableArray alloc] init];
    __block NSMutableArray *trackIds = [[NSMutableArray alloc] init];
    
    NSMutableArray *postEntryIds = [[NSMutableArray alloc] init];
    NSMutableArray *postTrackIds = [[NSMutableArray alloc] init];
    
    sqlite3_stmt *st = stmt[stmt_select_playlist];
    
    [indexSet enumerateIndexesUsingBlock:^(NSUInteger idx, BOOL * _Nonnull stop) {
        if (sqlite3_reset(st) ||
            sqlite3_bind_int64(st, select_playlist_in_id, idx) ||
            sqlite3_step(st) != SQLITE_ROW)
        {
            *stop = YES;
            return;
        }
        
        int64_t entryId = sqlite3_column_int64(st, select_playlist_out_entry_id);
        int64_t trackId = sqlite3_column_int64(st, select_playlist_out_track_id);
        
        [entryIds addObject:[NSNumber numberWithInteger:entryId]];
        [trackIds addObject:[NSNumber numberWithInteger:trackId]];
    }];
    
    int64_t count = [self playlistGetCount];
    
    sqlite3_reset(st);
    
    st = stmt[stmt_select_playlist_range];
    
    int rc;
    
    if (sqlite3_reset(st) ||
        sqlite3_bind_int64(st, select_playlist_range_in_id_low, insertIndex) ||
        sqlite3_bind_int64(st, select_playlist_range_in_id_high, count - 1) ||
        (rc = sqlite3_step(st)) != SQLITE_ROW)
    {
        return;
    }
    
    do
    {
        int64_t entryId = sqlite3_column_int64(st, select_playlist_range_out_entry_id);
        int64_t trackId = sqlite3_column_int64(st, select_playlist_range_out_track_id);
        
        if (![entryIds containsObject:[NSNumber numberWithInteger:entryId]])
        {
            [postEntryIds addObject:[NSNumber numberWithInteger:entryId]];
            [postTrackIds addObject:[NSNumber numberWithInteger:trackId]];
        }
        
        rc = sqlite3_step(st);
    }
    while (rc == SQLITE_ROW);
    
    if (rc != SQLITE_DONE)
    {
        return;
    }
    
    int64_t i;
    
    const int64_t entryIndexBase = insertIndex + [entryIds count];
    
    st = stmt[stmt_update_playlist];

    for (i = 0, count = [postEntryIds count]; i < count; ++i)
    {
        int64_t entryId = [[postEntryIds objectAtIndex:i] integerValue];
        int64_t trackId = [[postTrackIds objectAtIndex:i] integerValue];
        
        if (sqlite3_reset(st) ||
            sqlite3_bind_int64(st, update_playlist_in_entry_index, entryIndexBase + i) ||
            sqlite3_bind_int64(st, update_playlist_in_track_id, trackId) ||
            sqlite3_bind_int64(st, update_playlist_in_id, entryId) ||
            sqlite3_step(st) != SQLITE_DONE)
        {
            sqlite3_reset(st);
            return;
        }
    }

    for (i = 0, count = [entryIds count]; i < count; ++i)
    {
        int64_t entryId = [[entryIds objectAtIndex:i] integerValue];
        int64_t trackId = [[trackIds objectAtIndex:i] integerValue];
        
        if (sqlite3_reset(st) ||
            sqlite3_bind_int64(st, update_playlist_in_entry_index, insertIndex + i) ||
            sqlite3_bind_int64(st, update_playlist_in_track_id, trackId) ||
            sqlite3_bind_int64(st, update_playlist_in_id, entryId) ||
            sqlite3_step(st) != SQLITE_DONE)
        {
            return;
        }
    }

    sqlite3_reset(st);
}
#endif

- (void)syncPlaylistEntries:(NSArray *)entries progressCall:(void (^)(double))callback
{
    if (!entries || ![entries count])
    {
        callback(-1);
        return;
    }
    
    int64_t count = [self playlistGetCount];
    
    if (count != [entries count])
    {
        callback(-1);
        return;
    }
    
    callback(0);
    
    double progress = 0;
    double progressstep = 100.0 / (double)(count);
    
    sqlite3_stmt *st = stmt[stmt_update_playlist];
    
    for (size_t i = 0; i < count; ++i)
    {
        PlaylistEntry * newpe = [entries objectAtIndex:i];
        PlaylistEntry * oldpe = [databaseMirror objectAtIndex:i];

        progress += progressstep;
        
        if (([oldpe index] != i ||
            [oldpe dbIndex] != [newpe dbIndex]) &&
            [oldpe entryId] == [newpe entryId]) {
            
            if (sqlite3_reset(st) ||
                sqlite3_bind_int64(st, update_playlist_in_id, [oldpe entryId]) ||
                sqlite3_bind_int64(st, update_playlist_in_entry_index, i) ||
                sqlite3_bind_int64(st, update_playlist_in_track_id, [newpe dbIndex]) ||
                sqlite3_step(st) != SQLITE_ROW ||
                sqlite3_reset(st))
            {
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

- (void)queueAddItem:(int64_t)playlistIndex
{
    int64_t count = [self queueGetCount];
    
    sqlite3_stmt *st = stmt[stmt_add_queue];
    
    if (sqlite3_reset(st) ||
        sqlite3_bind_int64(st, add_queue_in_queue_index, count) ||
        sqlite3_bind_int64(st, add_queue_in_entry_id, playlistIndex) ||
        sqlite3_step(st) != SQLITE_DONE ||
        sqlite3_reset(st))
    {
        return;
    }
}

- (void)queueAddItems:(NSArray *)playlistIndexes
{
    int64_t count = [self queueGetCount];
    
    sqlite3_stmt *st = stmt[stmt_add_queue];
    
    for (NSNumber *index in playlistIndexes)
    {
        if (sqlite3_reset(st) ||
            sqlite3_bind_int64(st, add_queue_in_queue_index, count) ||
            sqlite3_bind_int64(st, add_queue_in_entry_id, [index integerValue]) ||
            sqlite3_step(st) != SQLITE_DONE)
        {
            break;
        }
        
        ++count;
    }
    
    sqlite3_reset(st);
}

- (void)queueRemoveItem:(int64_t)queueIndex
{
    sqlite3_stmt *st = stmt[stmt_remove_queue_by_index];
    
    if (sqlite3_reset(st) ||
        sqlite3_bind_int64(st, remove_queue_by_index_in_queue_index, queueIndex) ||
        sqlite3_step(st) != SQLITE_DONE ||
        sqlite3_reset(st))
    {
        return;
    }
    
    st = stmt[stmt_decrement_queue_for_removal];
    
    if (sqlite3_reset(st) ||
        sqlite3_bind_int64(st, decrement_queue_for_removal_in_index, queueIndex + 1) ||
        sqlite3_step(st) != SQLITE_DONE ||
        sqlite3_reset(st))
    {
        return;
    }
}

- (void)queueRemovePlaylistItems:(NSArray *)playlistIndexes
{
    sqlite3_stmt *st = stmt[stmt_select_queue_by_playlist_entry];
    
    for (NSNumber *index in playlistIndexes)
    {
        if (sqlite3_reset(st) ||
            sqlite3_bind_int64(st, select_queue_by_playlist_entry_in_id, [index integerValue]) ||
            sqlite3_step(st) != SQLITE_ROW)
        {
            break;
        }
        
        int64_t queueIndex = sqlite3_column_int64(st, select_queue_by_playlist_entry_out_queue_index);
        
        sqlite3_reset(st);
        
        [self queueRemoveItem:queueIndex];
    }
}

- (int64_t)queueGetEntry:(int64_t)queueIndex
{
    sqlite3_stmt *st = stmt[stmt_select_queue];
    
    if (sqlite3_reset(st) ||
        sqlite3_bind_int64(st, select_queue_in_id, queueIndex))
    {
        return -1;
    }
    
    int rc = sqlite3_step(st);
    
    if (rc != SQLITE_ROW && rc != SQLITE_DONE)
    {
        sqlite3_reset(st);
        return -1;
    }
    
    int64_t ret = -1;
    
    if (rc == SQLITE_ROW)
    {
        ret = sqlite3_column_int64(st, select_queue_out_entry_id);
    }
    
    sqlite3_reset(st);
    
    return ret;
}

- (void)queueEmpty
{
    sqlite3_stmt *st = stmt[stmt_remove_queue_all];

    if (sqlite3_reset(st) ||
        sqlite3_step(st) != SQLITE_DONE ||
        sqlite3_reset(st))
    {
        return;
    }
}

- (int64_t)queueGetCount
{
    sqlite3_stmt *st = stmt[stmt_count_queue];
    
    if (sqlite3_reset(st) ||
        sqlite3_step(st) != SQLITE_ROW)
    {
        return 0;
    }
    
    int64_t ret = sqlite3_column_int64(st, count_queue_out_count);
    
    sqlite3_reset(st);
    
    return ret;
}

@end
