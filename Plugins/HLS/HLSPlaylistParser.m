//
//  HLSPlaylistParser.m
//  HLS
//
//  Created by Claude on 2026-05-05.
//  Mildly edited by Christopher Snowhill on 2026-05-05
//  Copyright 2026 __LoSnoCo__. All rights reserved.
//

#import "HLSPlaylistParser.h"
#import "HLSPlaylist.h"
#import "HLSSegment.h"
#import "HLSVariant.h"
#import "Plugin.h"
#import "Logging.h"

@implementation HLSPlaylistParser

+ (HLSPlaylist *)parsePlaylistFromSource:(id<CogSource>)source error:(NSError **)error {
	NSMutableData *playlistData = [NSMutableData data];
	char *buffer = malloc(4096);
	long bytesRead;

	if (!buffer) {
		if (error) {
			*error = [NSError errorWithDomain:@"HLSParserError"
			                             code:1
			                         userInfo:@{NSLocalizedDescriptionKey: @"Failed to allocate buffer"}];
		}
		return nil;
	}

	while ((bytesRead = [source read:buffer amount:4096]) > 0) {
		[playlistData appendBytes:buffer length:bytesRead];
	}

	NSString *playlistString = [[NSString alloc] initWithData:playlistData encoding:NSUTF8StringEncoding];

	free(buffer);

	if (!playlistString) {
		if (error) {
			*error = [NSError errorWithDomain:@"HLSParserError"
			                             code:1
			                         userInfo:@{NSLocalizedDescriptionKey: @"Failed to decode playlist as UTF-8"}];
		}
		return nil;
	}

	return [self parsePlaylistString:playlistString baseURL:[source url] error:error];
}

+ (HLSPlaylist *)parsePlaylistString:(NSString *)playlistString baseURL:(NSURL *)baseURL error:(NSError **)error {
	HLSPlaylist *playlist = [[HLSPlaylist alloc] init];
	playlist.url = baseURL;
	playlist.baseURL = baseURL;

	NSMutableArray *segments = [NSMutableArray array];
	NSMutableArray *variants = [NSMutableArray array];

	NSArray *lines = [playlistString componentsSeparatedByCharactersInSet:
		[NSCharacterSet newlineCharacterSet]];

	HLSSegment *currentSegment = nil;
	HLSVariant *currentVariant = nil;
	BOOL isMaster = NO;
	BOOL hasValidHeader = NO;

	for (NSString *line in lines) {
		NSString *trimmedLine = [line stringByTrimmingCharactersInSet:
			[NSCharacterSet whitespaceAndNewlineCharacterSet]];

		if ([trimmedLine length] == 0) {
			continue;
		}

		if ([trimmedLine hasPrefix:@"#EXTM3U"]) {
			hasValidHeader = YES;
			continue;
		}

		if (!hasValidHeader) {
			if (error) {
				*error = [NSError errorWithDomain:@"HLSParserError"
				                             code:2
				                         userInfo:@{NSLocalizedDescriptionKey: @"Invalid m3u8 playlist - missing #EXTM3U header"}];
			}
			return nil;
		}

		if ([trimmedLine hasPrefix:@"#EXT-X-VERSION:"]) {
			NSString *versionStr = [[trimmedLine componentsSeparatedByString:@":"] lastObject];
			playlist.version = [versionStr integerValue];
		}
		else if ([trimmedLine hasPrefix:@"#EXT-X-STREAM-INF:"]) {
			isMaster = YES;
			currentVariant = [[HLSVariant alloc] init];
			currentVariant.playlistURL = baseURL;

			NSString *attributes = [trimmedLine substringFromIndex:18];

			NSArray *attrPairs = [attributes componentsSeparatedByString:@","];
			for (NSString *pair in attrPairs) {
				NSString *trimmedPair = [pair stringByTrimmingCharactersInSet:
					[NSCharacterSet whitespaceAndNewlineCharacterSet]];

				if ([trimmedPair hasPrefix:@"BANDWIDTH="]) {
					NSString *bwStr = [[trimmedPair componentsSeparatedByString:@"="] lastObject];
					currentVariant.bandwidth = [bwStr integerValue];
				}
				else if ([trimmedPair hasPrefix:@"AVERAGE-BANDWIDTH="]) {
					NSString *bwStr = [[trimmedPair componentsSeparatedByString:@"="] lastObject];
					currentVariant.averageBandwidth = [bwStr integerValue];
				}
				else if ([trimmedPair hasPrefix:@"CODECS=\""]) {
					NSRange startRange = [trimmedPair rangeOfString:@"CODECS=\"" options:0];
					if (startRange.location != NSNotFound) {
						NSString *codecsStr = [trimmedPair substringFromIndex:startRange.location + startRange.length];
						NSRange endRange = [codecsStr rangeOfString:@"\"" options:0];
						if (endRange.location != NSNotFound) {
							currentVariant.codecs = [codecsStr substringToIndex:endRange.location];
						}
					}
				}
				else if ([trimmedPair hasPrefix:@"RESOLUTION="]) {
					NSString *resStr = [[trimmedPair componentsSeparatedByString:@"="] lastObject];
					currentVariant.resolution = [resStr stringByReplacingOccurrencesOfString:@"\"" withString:@""];
				}
			}
		}
		else if ([trimmedLine hasPrefix:@"#EXT-X-TARGETDURATION:"]) {
			NSString *durationStr = [[trimmedLine componentsSeparatedByString:@":"] lastObject];
			playlist.targetDuration = [durationStr integerValue];
		}
		else if ([trimmedLine hasPrefix:@"#EXT-X-MEDIA-SEQUENCE:"]) {
			NSString *seqStr = [[trimmedLine componentsSeparatedByString:@":"] lastObject];
			playlist.mediaSequence = [seqStr integerValue];
		}
		else if ([trimmedLine hasPrefix:@"#EXT-X-PLAYLIST-TYPE:"]) {
			NSString *typeStr = [[trimmedLine componentsSeparatedByString:@":"] lastObject];
			if ([typeStr isEqualToString:@"EVENT"]) {
				playlist.type = HLSPlaylistTypeEvent;
			}
			else if ([typeStr isEqualToString:@"VOD"]) {
				playlist.type = HLSPlaylistTypeVOD;
			}
		}
		else if ([trimmedLine hasPrefix:@"#EXTINF:"]) {
			currentSegment = [[HLSSegment alloc] init];

			NSString *durationPart = [trimmedLine substringFromIndex:8];
			NSArray *parts = [durationPart componentsSeparatedByString:@","];
			NSString *durationStr = parts[0];
			currentSegment.duration = [durationStr doubleValue];
		}
		else if ([trimmedLine hasPrefix:@"#EXT-X-KEY:"]) {
			if (currentSegment) {
				NSString *attributes = [trimmedLine substringFromIndex:11];

				if ([attributes rangeOfString:@"METHOD=NONE"].location != NSNotFound) {
					currentSegment.encrypted = NO;
				}
				else if ([attributes rangeOfString:@"METHOD=AES-128"].location != NSNotFound ||
				         [attributes rangeOfString:@"METHOD=SAMPLE-AES"].location != NSNotFound) {
					currentSegment.encrypted = YES;
					DLog(@"HLS: Encrypted segments detected - not supported in this version");
				}
			}
		}
		else if ([trimmedLine isEqualToString:@"#EXT-X-DISCONTINUITY"]) {
			if (currentSegment) {
				currentSegment.discontinuity = YES;
			}
		}
		else if ([trimmedLine isEqualToString:@"#EXT-X-ENDLIST"]) {
			playlist.isLiveStream = NO;
		}
		else if ([trimmedLine hasPrefix:@"#EXT-X-MAP:"]) {
			if (currentSegment) {
				NSString *attributes = [trimmedLine substringFromIndex:11];
				NSRange uriRange = [attributes rangeOfString:@"URI=\""];
				if (uriRange.location != NSNotFound) {
					NSString *uriPart = [attributes substringFromIndex:uriRange.location + uriRange.length];
					NSRange endRange = [uriPart rangeOfString:@"\""];
					if (endRange.location != NSNotFound) {
						NSString *uriString = [uriPart substringToIndex:endRange.location];
						currentSegment.mapSectionURL = [NSURL URLWithString:uriString relativeToURL:baseURL];
					}
				}
			}
		}
		else if (![trimmedLine hasPrefix:@"#"]) {
			NSURL *itemURL = [NSURL URLWithString:trimmedLine];

			if (itemURL == nil || ![itemURL scheme]) {
				itemURL = [NSURL URLWithString:trimmedLine relativeToURL:baseURL];
			}

			if (currentVariant) {
				currentVariant.url = itemURL;
				[variants addObject:currentVariant];
				currentVariant = nil;
			}
			else if (currentSegment) {
				currentSegment.url = itemURL;
				currentSegment.sequenceNumber = playlist.mediaSequence + [segments count];
				[segments addObject:currentSegment];
				currentSegment = nil;
			}
		}
	}

	if (!hasValidHeader) {
		if (error) {
			*error = [NSError errorWithDomain:@"HLSParserError"
			                             code:2
			                         userInfo:@{NSLocalizedDescriptionKey: @"Invalid m3u8 playlist - missing #EXTM3U header"}];
		}
		return nil;
	}

	playlist.isMasterPlaylist = isMaster;

	// Deduplicate segments based on full URL (including query string)
	NSMutableArray *deduplicatedSegments = [NSMutableArray array];
	NSMutableSet<NSString *> *seenURLs = [NSMutableSet set];
	NSInteger duplicatesRemoved = 0;

	for (HLSSegment *segment in segments) {
		NSString *fullURL = [segment.url absoluteString];
		if (![seenURLs containsObject:fullURL]) {
			[seenURLs addObject:fullURL];
			[deduplicatedSegments addObject:segment];
		} else {
			DLog(@"HLS: Skipping duplicate segment: %@", fullURL);
			duplicatesRemoved++;
		}
	}

	playlist.segments = deduplicatedSegments;
	playlist.variants = variants;

	DLog(@"HLS: Parsed playlist - isMaster: %d, isLive: %d, segments: %lu, duplicates removed: %ld, final: %lu, variants: %lu",
	     isMaster, playlist.isLiveStream, (unsigned long)[segments count], (long)duplicatesRemoved,
	     (unsigned long)[deduplicatedSegments count], (unsigned long)[variants count]);

	return playlist;
}

@end
