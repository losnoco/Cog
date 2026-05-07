//
//  HLSSegment.h
//  HLS
//
//  Created by Claude on 2026-05-05.
//  Rewritten by Christopher Snowhill on 2026-05-06
//  Copyright 2026 __LoSnoCo__. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface HLSSegment : NSObject

@property (nonatomic, strong) NSURL *url;
@property (nonatomic) double duration;
@property (nonatomic) NSInteger sequenceNumber;
@property (nonatomic) NSInteger discontinuitySequence;
@property (nonatomic) BOOL discontinuity;

@property (nonatomic) BOOL encrypted;
@property (nonatomic, copy) NSString *encryptionMethod;
@property (nonatomic, strong) NSURL *encryptionKeyURL;
@property (nonatomic, strong) NSData *iv;

@property (nonatomic, strong) NSURL *mapSectionURL;
@property (nonatomic, copy) NSString *title;

// Filled in by the segment manager once data has been fetched.
@property (nonatomic, copy) NSString *mimeType;

@end
