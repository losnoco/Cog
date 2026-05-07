//
//  HLSPlaylistParser.m
//  HLS
//
//  Created by Claude on 2026-05-05.
//  Rewritten by Christopher Snowhill on 2026-05-06
//  Copyright 2026 __LoSnoCo__. All rights reserved.
//
//  RFC 8216 compliant m3u8 parser. Handles attribute lists with quoted
//  strings (so CODECS="mp4a.40.2,avc1.42E01E" parses correctly), forward-
//  applied EXT-X-KEY / EXT-X-MAP, EXT-X-DISCONTINUITY-SEQUENCE, and the
//  usual VOD / live distinction.
//

#import "HLSPlaylistParser.h"
#import "HLSPlaylist.h"
#import "HLSSegment.h"
#import "HLSVariant.h"
#import "Logging.h"

NSString *const HLSParserErrorDomain = @"HLSParserErrorDomain";

@implementation HLSPlaylistParser

#pragma mark - Helpers

// Read the full contents of a CogSource into memory, then decode as UTF-8.
// HLS playlists are required to be UTF-8 (RFC 8216 §4).
+ (NSString *)readPlaylistString:(id<CogSource>)source error:(NSError **)error {
	NSMutableData *data = [NSMutableData data];
	uint8_t buffer[4096];
	long bytesRead;

	while((bytesRead = [source read:buffer amount:sizeof(buffer)]) > 0) {
		[data appendBytes:buffer length:bytesRead];
	}

	if([data length] == 0) {
		if(error) {
			*error = [NSError errorWithDomain:HLSParserErrorDomain
			                             code:HLSParserErrorReadFailed
			                         userInfo:@{NSLocalizedDescriptionKey: @"Failed to read playlist"}];
		}
		return nil;
	}

	// Strip a leading UTF-8 BOM if the publisher accidentally added one.
	const uint8_t *bytes = [data bytes];
	NSUInteger length = [data length];
	NSUInteger offset = 0;
	if(length >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF) {
		offset = 3;
	}

	NSString *str = [[NSString alloc] initWithBytes:bytes + offset
	                                         length:length - offset
	                                       encoding:NSUTF8StringEncoding];
	if(!str) {
		// Fallback: try ISO Latin-1 so we at least don't fail outright.
		str = [[NSString alloc] initWithBytes:bytes + offset
		                               length:length - offset
		                             encoding:NSISOLatin1StringEncoding];
	}

	if(!str && error) {
		*error = [NSError errorWithDomain:HLSParserErrorDomain
		                             code:HLSParserErrorEncoding
		                         userInfo:@{NSLocalizedDescriptionKey: @"Failed to decode playlist text"}];
	}
	return str;
}

// Parse an attribute-list per RFC 8216 §4.2:
//     attribute-list  = attribute *( "," attribute )
//     attribute       = name "=" value
//     value           = decimal-int | hex-int | float | quoted-string |
//                       enumerated-string | resolution
// Splits at top-level commas (not inside double quotes), and strips quotes
// from quoted values. Returns the dict keyed by the (UPPERCASE) attribute
// name, since HLS attribute names are case-sensitive but always uppercase
// in practice. We don't lowercase keys here.
+ (NSDictionary<NSString *, NSString *> *)parseAttributeList:(NSString *)attrs {
	NSMutableDictionary<NSString *, NSString *> *out = [NSMutableDictionary dictionary];
	NSUInteger len = [attrs length];
	NSUInteger i = 0;

	while(i < len) {
		// Skip leading whitespace before the key.
		while(i < len) {
			unichar c = [attrs characterAtIndex:i];
			if(c == ' ' || c == '\t') {
				i++;
			} else {
				break;
			}
		}
		if(i >= len) break;

		// Read key up to '=' or ','.
		NSUInteger keyStart = i;
		while(i < len) {
			unichar c = [attrs characterAtIndex:i];
			if(c == '=' || c == ',') break;
			i++;
		}
		NSString *key = [[attrs substringWithRange:NSMakeRange(keyStart, i - keyStart)]
		    stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];

		NSString *value = @"";
		if(i < len && [attrs characterAtIndex:i] == '=') {
			i++; // skip '='
			if(i < len && [attrs characterAtIndex:i] == '"') {
				// Quoted string. Read up to the closing quote, no escape
				// sequence per RFC 8216 (the value cannot contain ").
				i++;
				NSUInteger valStart = i;
				while(i < len && [attrs characterAtIndex:i] != '"') {
					i++;
				}
				value = [attrs substringWithRange:NSMakeRange(valStart, i - valStart)];
				if(i < len) i++; // skip closing quote
			} else {
				// Unquoted: read up to ','.
				NSUInteger valStart = i;
				while(i < len && [attrs characterAtIndex:i] != ',') {
					i++;
				}
				value = [[attrs substringWithRange:NSMakeRange(valStart, i - valStart)]
				    stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
			}
		}

		if([key length]) {
			out[key] = value;
		}

		// Skip a trailing comma so the next iteration starts on the next
		// attribute.
		if(i < len && [attrs characterAtIndex:i] == ',') i++;
	}

	return out;
}

// HLS hex literals look like 0xABCDEF... -- decode into bytes.
+ (NSData *)dataFromHexString:(NSString *)hex {
	NSString *s = hex;
	if([s hasPrefix:@"0x"] || [s hasPrefix:@"0X"]) {
		s = [s substringFromIndex:2];
	}
	if([s length] % 2 == 1) {
		s = [@"0" stringByAppendingString:s];
	}

	NSMutableData *data = [NSMutableData dataWithCapacity:[s length] / 2];
	for(NSUInteger i = 0; i + 2 <= [s length]; i += 2) {
		NSString *byteStr = [s substringWithRange:NSMakeRange(i, 2)];
		unsigned int byteVal = 0;
		NSScanner *scanner = [NSScanner scannerWithString:byteStr];
		if(![scanner scanHexInt:&byteVal]) {
			return nil;
		}
		uint8_t b = (uint8_t)byteVal;
		[data appendBytes:&b length:1];
	}
	return data;
}

+ (NSURL *)resolveURL:(NSString *)uri againstBaseURL:(NSURL *)baseURL {
	NSString *trimmed = [uri stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
	if([trimmed length] == 0) return nil;

	NSURL *url = [NSURL URLWithString:trimmed relativeToURL:baseURL];
	if(!url) {
		// Fallback: percent-encode, in case the URI has spaces or other
		// characters that NSURL refuses to parse.
		NSString *escaped = [trimmed stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet URLQueryAllowedCharacterSet]];
		url = [NSURL URLWithString:escaped relativeToURL:baseURL];
	}
	return [url absoluteURL];
}

#pragma mark - Public API

+ (HLSPlaylist *)parsePlaylistFromSource:(id<CogSource>)source error:(NSError **)error {
	NSString *str = [self readPlaylistString:source error:error];
	if(!str) return nil;
	return [self parsePlaylistString:str baseURL:[source url] error:error];
}

+ (HLSPlaylist *)parsePlaylistString:(NSString *)playlistString
                             baseURL:(NSURL *)baseURL
                               error:(NSError **)error {
	HLSPlaylist *playlist = [[HLSPlaylist alloc] init];
	playlist.url = baseURL;

	NSMutableArray<HLSSegment *> *segments = [NSMutableArray array];
	NSMutableArray<HLSVariant *> *variants = [NSMutableArray array];

	HLSSegment *pendingSegment = nil;        // EXTINF -> URI
	HLSVariant *pendingVariant = nil;        // EXT-X-STREAM-INF -> URI

	// Forward-applied state: tags before #EXTINF apply to the next segment
	// AND all subsequent segments until another tag of the same kind
	// supersedes them.
	NSURL *currentMapURL = nil;
	BOOL keyEncrypted = NO;
	NSString *keyMethod = nil;
	NSURL *keyURL = nil;
	NSData *keyIV = nil;
	BOOL pendingDiscontinuity = NO;

	BOOL sawHeader = NO;
	BOOL isMaster = NO;

	NSArray<NSString *> *lines = [playlistString componentsSeparatedByCharactersInSet:
	    [NSCharacterSet newlineCharacterSet]];

	for(NSString *rawLine in lines) {
		NSString *line = [rawLine stringByTrimmingCharactersInSet:
		    [NSCharacterSet whitespaceAndNewlineCharacterSet]];
		if([line length] == 0) continue;

		if(!sawHeader) {
			if([line isEqualToString:@"#EXTM3U"]) {
				sawHeader = YES;
				continue;
			} else {
				if(error) {
					*error = [NSError errorWithDomain:HLSParserErrorDomain
					                             code:HLSParserErrorMissingHeader
					                         userInfo:@{NSLocalizedDescriptionKey: @"Missing #EXTM3U header"}];
				}
				return nil;
			}
		}

		// URI line (no leading '#') -- terminates a pending segment or
		// variant declaration.
		if(![line hasPrefix:@"#"]) {
			NSURL *url = [self resolveURL:line againstBaseURL:baseURL];
			if(!url) {
				DLog(@"HLS: ignoring unparseable URI line: %@", line);
				continue;
			}

			if(pendingVariant) {
				pendingVariant.url = url;
				[variants addObject:pendingVariant];
				pendingVariant = nil;
				isMaster = YES;
			} else if(pendingSegment) {
				pendingSegment.url = url;
				pendingSegment.sequenceNumber = playlist.mediaSequence + [segments count];
				pendingSegment.discontinuitySequence = playlist.discontinuitySequence;
				if(currentMapURL) pendingSegment.mapSectionURL = currentMapURL;
				if(keyEncrypted) {
					pendingSegment.encrypted = YES;
					pendingSegment.encryptionMethod = keyMethod;
					pendingSegment.encryptionKeyURL = keyURL;
					pendingSegment.iv = keyIV;
				}
				if(pendingDiscontinuity) {
					pendingSegment.discontinuity = YES;
					playlist.discontinuitySequence++;
					pendingDiscontinuity = NO;
				}
				[segments addObject:pendingSegment];
				pendingSegment = nil;
			} else {
				DLog(@"HLS: stray URI line (no preceding #EXTINF or #EXT-X-STREAM-INF): %@", line);
			}
			continue;
		}

		// At this point we have a tag line. Find the colon that separates
		// the tag name from its value, but allow tags without a value
		// (e.g. #EXT-X-ENDLIST).
		NSRange colon = [line rangeOfString:@":"];
		NSString *tag;
		NSString *value;
		if(colon.location == NSNotFound) {
			tag = line;
			value = @"";
		} else {
			tag = [line substringToIndex:colon.location];
			value = [line substringFromIndex:colon.location + 1];
		}

		if([tag isEqualToString:@"#EXT-X-VERSION"]) {
			playlist.version = [value integerValue];
		} else if([tag isEqualToString:@"#EXT-X-TARGETDURATION"]) {
			playlist.targetDuration = [value integerValue];
		} else if([tag isEqualToString:@"#EXT-X-MEDIA-SEQUENCE"]) {
			playlist.mediaSequence = [value integerValue];
		} else if([tag isEqualToString:@"#EXT-X-DISCONTINUITY-SEQUENCE"]) {
			playlist.discontinuitySequence = [value integerValue];
		} else if([tag isEqualToString:@"#EXT-X-PLAYLIST-TYPE"]) {
			NSString *t = [value stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
			if([t caseInsensitiveCompare:@"VOD"] == NSOrderedSame) {
				playlist.type = HLSPlaylistTypeVOD;
				playlist.isLiveStream = NO;
			} else if([t caseInsensitiveCompare:@"EVENT"] == NSOrderedSame) {
				playlist.type = HLSPlaylistTypeEvent;
				// EVENT playlists are still "live" in the sense that more
				// segments may be added; isLiveStream stays YES.
			}
		} else if([tag isEqualToString:@"#EXT-X-ENDLIST"]) {
			playlist.hasEndList = YES;
			playlist.isLiveStream = NO;
		} else if([tag isEqualToString:@"#EXTINF"]) {
			// "#EXTINF:duration[,title]"
			pendingSegment = [[HLSSegment alloc] init];
			NSRange comma = [value rangeOfString:@","];
			NSString *durationStr = (comma.location == NSNotFound) ? value
			                                                       : [value substringToIndex:comma.location];
			pendingSegment.duration = [durationStr doubleValue];
			if(comma.location != NSNotFound && comma.location + 1 <= [value length]) {
				pendingSegment.title = [value substringFromIndex:comma.location + 1];
			}
		} else if([tag isEqualToString:@"#EXT-X-DISCONTINUITY"]) {
			pendingDiscontinuity = YES;
		} else if([tag isEqualToString:@"#EXT-X-KEY"]) {
			NSDictionary<NSString *, NSString *> *attrs = [self parseAttributeList:value];
			NSString *method = attrs[@"METHOD"];
			if(!method || [method caseInsensitiveCompare:@"NONE"] == NSOrderedSame) {
				keyEncrypted = NO;
				keyMethod = nil;
				keyURL = nil;
				keyIV = nil;
			} else {
				keyEncrypted = YES;
				keyMethod = method;
				NSString *uri = attrs[@"URI"];
				keyURL = uri ? [self resolveURL:uri againstBaseURL:baseURL] : nil;
				NSString *iv = attrs[@"IV"];
				keyIV = iv ? [self dataFromHexString:iv] : nil;
				DLog(@"HLS: encrypted segments declared (method=%@) -- not supported", method);
			}
		} else if([tag isEqualToString:@"#EXT-X-MAP"]) {
			NSDictionary<NSString *, NSString *> *attrs = [self parseAttributeList:value];
			NSString *uri = attrs[@"URI"];
			currentMapURL = uri ? [self resolveURL:uri againstBaseURL:baseURL] : nil;
		} else if([tag isEqualToString:@"#EXT-X-STREAM-INF"]) {
			pendingVariant = [[HLSVariant alloc] init];
			pendingVariant.playlistURL = baseURL;
			NSDictionary<NSString *, NSString *> *attrs = [self parseAttributeList:value];
			pendingVariant.bandwidth = [attrs[@"BANDWIDTH"] integerValue];
			pendingVariant.averageBandwidth = [attrs[@"AVERAGE-BANDWIDTH"] integerValue];
			pendingVariant.codecs = attrs[@"CODECS"];
			pendingVariant.resolution = attrs[@"RESOLUTION"];
		} else if([tag isEqualToString:@"#EXT-X-MEDIA"]
		          || [tag isEqualToString:@"#EXT-X-I-FRAME-STREAM-INF"]
		          || [tag isEqualToString:@"#EXT-X-INDEPENDENT-SEGMENTS"]
		          || [tag isEqualToString:@"#EXT-X-START"]
		          || [tag isEqualToString:@"#EXT-X-PROGRAM-DATE-TIME"]
		          || [tag isEqualToString:@"#EXT-X-BYTERANGE"]
		          || [tag isEqualToString:@"#EXT-X-ALLOW-CACHE"]
		          || [tag isEqualToString:@"#EXT-X-GAP"]
		          || [tag isEqualToString:@"#EXT-X-BITRATE"]) {
			// Parsed but not used yet. Recognized so the parser doesn't
			// log "unknown tag" noise.
		} else if([tag hasPrefix:@"#EXT"]) {
			DLog(@"HLS: ignoring unrecognized tag: %@", tag);
		}
		// Plain `#` comment lines are silently ignored.
	}

	if(!sawHeader) {
		if(error) {
			*error = [NSError errorWithDomain:HLSParserErrorDomain
			                             code:HLSParserErrorMissingHeader
			                         userInfo:@{NSLocalizedDescriptionKey: @"Missing #EXTM3U header"}];
		}
		return nil;
	}

	if([segments count] == 0 && [variants count] == 0) {
		if(error) {
			*error = [NSError errorWithDomain:HLSParserErrorDomain
			                             code:HLSParserErrorEmpty
			                         userInfo:@{NSLocalizedDescriptionKey: @"Playlist contains no segments or variants"}];
		}
		return nil;
	}

	playlist.isMasterPlaylist = isMaster;
	playlist.segments = segments;
	playlist.variants = variants;

	DLog(@"HLS: parsed playlist (master=%d, live=%d, segments=%lu, variants=%lu, target=%ld)",
	     isMaster, playlist.isLiveStream,
	     (unsigned long)[segments count], (unsigned long)[variants count],
	     (long)playlist.targetDuration);

	return playlist;
}

@end
