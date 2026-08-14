//
//  CogURLNormalization.h
//  Cog
//
//  Created by Christopher Snowhill on 8/13/26.
//

#ifndef CogURLNormalization_h
#define CogURLNormalization_h

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/* Dropping a file onto the player, or receiving one from certain services,
 * can hand us a macOS file reference URL:
 *
 *     file:///.file/id=6571714.6571725
 *
 * These identify a file by volume and inode rather than by path. They resolve
 * only for as long as the file exists at that inode, they never survive a move
 * to another volume, and most of the player is plain C that ends up calling
 * fopen() on the URL's path, which a reference URL cannot supply once it goes
 * stale. Nothing may be opened or stored in that form: normalize first. */

/// Cheap syntactic test against a serialized URL or a bare filesystem path.
/// Does not touch the file system.
static inline BOOL CogURLStringIsFileReference(NSString *_Nullable string) {
	if(!string) return NO;
	return [string hasPrefix:@"file:///.file/id="] || [string hasPrefix:@"/.file/id="];
}

/// Resolves a file reference URL to a plain path URL, preserving any fragment
/// the player appended (cue sheet track numbers). Any other URL, including a
/// reference whose file no longer exists, is returned unchanged.
static inline NSURL *_Nullable CogNormalizeURL(NSURL *_Nullable url) {
	if(!url || ![url isFileReferenceURL]) return url;

	NSURL *resolved = [url filePathURL];
	return resolved ? resolved : url;
}

/// Same, for a bare filesystem path. Note that +[NSURL fileURLWithPath:] will
/// not produce a reference URL, so the string has to be parsed as a URL to get
/// one back that can be resolved.
static inline NSString *_Nullable CogNormalizeFilePath(NSString *_Nullable path) {
	if(!path || ![path hasPrefix:@"/.file/id="]) return path;

	NSURL *url = [NSURL URLWithString:[@"file://" stringByAppendingString:path]];
	NSString *resolved = [CogNormalizeURL(url) path];
	return resolved ? resolved : path;
}

/// Same, for a serialized URL as stored in the playlist database. Returns nil
/// if the string did not need normalizing, so callers can cheaply tell whether
/// they have anything to write back.
static inline NSString *_Nullable CogNormalizeURLStringIfNeeded(NSString *_Nullable string) {
	if(!CogURLStringIsFileReference(string)) return nil;

	NSURL *url = [string hasPrefix:@"/"] ? [NSURL URLWithString:[@"file://" stringByAppendingString:string]] :
	                                       [NSURL URLWithString:string];
	if(!url) return nil;

	NSURL *resolved = CogNormalizeURL(url);
	if(resolved == url) return nil;

	NSString *normalized = [resolved absoluteString];
	return [normalized isEqualToString:string] ? nil : normalized;
}

NS_ASSUME_NONNULL_END

#endif /* CogURLNormalization_h */
