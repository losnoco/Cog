// Plugins! HOORAY!

#if __has_include(<CogAudio/AudioChunk.h>)
#    import <CogAudio/AudioChunk.h>
#else
#    import "AudioChunk.h"
#endif

@protocol CogSource <NSObject>
+ (NSArray *)schemes; // http, file, etc

- (NSURL *)url;
- (NSString *)mimeType;

- (BOOL)open:(NSURL *)url;
- (BOOL)seekable;
- (BOOL)seek:(long)position whence:(int)whence;
- (long)tell;
- (long)read:(void *)buffer amount:(long)amount; // reads UP TO amount, returns amount read.
- (void)close;

@optional
// Promptly unblock any in-flight read without waiting for full teardown.
// The normal -close call still follows on the reader's own thread.
- (void)interrupt;
// The files in the folder that holds `url`, or in that folder of its
// archive, by name, as URLs this source opens. Needs no open: first.
- (NSDictionary *)filesBesideURL:(NSURL *)url;
@end

/* ICY metadata from an HTTP stream. Decoders cast their source to this
 * protocol rather than naming the HTTPSource class, so the source can be
 * ported or replaced without touching them. */
@protocol CogHTTPSource <CogSource>
- (BOOL)hasMetadata;
- (NSDictionary *)metadata;
@end

@protocol CogVersionCheck <NSObject>
+ (BOOL)shouldLoadForOSVersion:(NSOperatingSystemVersion)version;
@end

@protocol CogContainer <NSObject>
+ (NSArray *)fileTypes; // mp3, ogg, etc
+ (NSArray *)mimeTypes;
+ (float)priority;

+ (NSArray *)urlsForContainerURL:(NSURL *)url;

@optional
+ (NSArray *)dependencyUrlsForContainerURL:(NSURL *)url;

// The tracks urlsForContainerURL: gives, and with them, in *info, what the
// properties and metadata readers would give each, merged as the playlist
// stores them, keyed by track URL: read in the one pass that finds the
// tracks, where the readers would open the file again for every one. A
// track left out of *info is read as usual. A container that finds no
// tracks may still describe the file itself, under its own URL, but only as
// the readers give it, having read it with them.
+ (NSArray *)urlsForContainerURL:(NSURL *)url trackInfo:(NSDictionary **)info;
@end

@protocol CogDecoder <NSObject>
@required
+ (NSArray *)mimeTypes;
+ (NSArray *)fileTypes; // mp3, ogg, etc
+ (NSArray *)fileTypeAssociations; // array of NSArray of NSString, where first item in array is the type name, the second is the icon name, and the rest are the extensions
+ (float)priority; // should be 0.0 ... 1.0, higher means you get selected first, should default to 1.0 unless you know a reason why any of your extensions may behave badly, ie. greedily taking over some file type extension without performing any header validation on it

// For KVO
//- (void)setProperties:(NSDictionary *)p;
- (NSDictionary *)properties;
- (NSDictionary *)metadata; // Only to be implemented for dynamic metadata, send events on change

- (AudioChunk *)readAudio;

- (BOOL)open:(id<CogSource>)source;
- (long)seek:(long)frame;
- (void)close;

@optional
// Promptly unblock -readAudio when playback is being stopped. Implementations
// should only interrupt their input here; decoder teardown remains in -close.
- (void)interrupt;

- (BOOL)setTrack:(NSURL *)track;

// These are in NSObject, so as long as you are a subclass of that, you are ok.
- (void)addObserver:(NSObject *)observer forKeyPath:(NSString *)keyPath options:(NSKeyValueObservingOptions)options context:(void *)context;
- (void)removeObserver:(NSObject *)observer forKeyPath:(NSString *)keyPath;
- (void)removeObserver:(NSObject *)observer forKeyPath:(NSString *)keyPath context:(void *)context;

- (BOOL)isSilence;
@end

@protocol CogMetadataReader <NSObject>
+ (NSArray *)fileTypes;
+ (NSArray *)mimeTypes;
+ (float)priority;
+ (NSDictionary *)metadataForURL:(NSURL *)url;
@end

// What a tag writer's probe says it can keep in one file. Missing or NO
// means it can't; a dictionary without CogTagWriterWritable means no writer
// can save the file at all.
static NSString *const CogTagWriterWritable = @"writable"; // tags can be saved at all
static NSString *const CogTagWriterArbitraryKeys = @"arbitraryKeys"; // any tag name keeps its own name
static NSString *const CogTagWriterFoldsKeys = @"foldsKeys"; // other names survive, folded (ID3v2 TXXX, MP4 ----)
static NSString *const CogTagWriterMultiValue = @"multiValue"; // a tag keeps two or more values
static NSString *const CogTagWriterPictures = @"pictures"; // embedded cover art
static NSString *const CogTagWriterChapters = @"chapters"; // chapters can be read and written

// In the dictionary a writer reads and writes: the cover as NSData (empty
// NSData removes it), and the chapters as an array of dictionaries with the
// keys below, ordered by start. Every other key is a tag's lowercased name
// holding an array of strings, named as CogMetadataReader names them; an
// empty array removes the tag.
static NSString *const CogTagAlbumArt = @"albumArt";
static NSString *const CogTagChapters = @"chapters";
static NSString *const CogChapterStart = @"start"; // NSNumber, milliseconds
static NSString *const CogChapterEnd = @"end"; // NSNumber, milliseconds
static NSString *const CogChapterTitle = @"title"; // NSString
static NSString *const CogChapterID = @"id"; // opaque, kept from a read so a chapter keeps its identity

@protocol CogMetadataWriter <NSObject>
+ (NSArray *)fileTypes;
+ (float)priority;
// The probe: opens the file and says what a save would keep.
+ (NSDictionary *)capabilitiesForURL:(NSURL *)url;
// The file's own tags as this writer would write them, with no other
// reader's and no folder art.
+ (NSDictionary *)tagsForURL:(NSURL *)url;
// replace: the dictionary is the file's whole set of tags, and any other is
// removed. Otherwise only the tags it names change. The cover and the
// chapters are left alone when absent either way.
+ (BOOL)writeTags:(NSDictionary *)tags toURL:(NSURL *)url replace:(BOOL)replace error:(NSError **)error;
@end

@protocol CogPropertiesReader <NSObject>
+ (NSArray *)fileTypes;
+ (NSArray *)mimeTypes;
+ (float)priority;
+ (NSDictionary *)propertiesForSource:(id<CogSource>)source;
@end

@protocol CogPluginController <NSObject>
+ (id<CogPluginController>)sharedPluginController;

- (NSDictionary *)sources;
- (NSDictionary *)containers;
- (NSDictionary *)metadataReaders;

- (NSDictionary *)propertiesReadersByExtension;
- (NSDictionary *)propertiesReadersByMimeType;

- (NSDictionary *)decodersByExtension;
- (NSDictionary *)decodersByMimeType;

- (id<CogSource>)audioSourceForURL:(NSURL *)url;
- (NSArray *)urlsForContainerURL:(NSURL *)url;
- (NSArray *)urlsForContainerURL:(NSURL *)url trackInfo:(NSDictionary **)info;
- (NSArray *)dependencyUrlsForContainerURL:(NSURL *)url;
- (NSDictionary *)metadataForURL:(NSURL *)url skipCue:(BOOL)skip;
- (NSDictionary *)propertiesForURL:(NSURL *)url skipCue:(BOOL)skip;
- (id<CogDecoder>)audioDecoderForSource:(id<CogSource>)source skipCue:(BOOL)skip;

// Tag writing. URLs with a fragment (cue tracks, subsongs, chapters) are
// refused: callers pass the file's own URL. The writer used is the
// highest-priority one for the extension whose probe says it can save.
- (NSDictionary *)metadataWriters;
- (NSDictionary *)tagWritingCapabilitiesForURL:(NSURL *)url;
- (NSDictionary *)writableTagsForURL:(NSURL *)url;
- (BOOL)writeTags:(NSDictionary *)tags toURL:(NSURL *)url replace:(BOOL)replace error:(NSError **)error;
// Drops what the tag cache holds for the file and its subsongs, so the next
// read after a save sees the file.
- (void)forgetMetadataForURL:(NSURL *)url;
@end

#ifdef __cplusplus
extern "C" {
#endif
extern NSString *guess_encoding_of_string(const char *input);
#ifdef __cplusplus
}
#endif

/* The app's sandbox broker, which plugins reach by name so they need not
 * link the app: cast NSClassFromString(@"SandboxBroker") to this protocol's
 * metatype. Where there is no broker (iOS), the class is absent and the
 * cast gives nil. */
@protocol CogSandboxBroker <NSObject>
+ (id<CogSandboxBroker>)sharedSandboxBroker;
- (const void *_Nullable)beginFolderAccess:(NSURL *)fileUrl;
- (void)endFolderAccess:(const void *_Nullable)handle;
@end
