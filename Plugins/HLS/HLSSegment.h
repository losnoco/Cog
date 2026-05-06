//
//  HLSSegment.h
//  HLS
//
//  Created by Claude on 2026-05-05.
//  Lightly edited by Christopher Snowhill on 2026-05-05
//  Copyright 2026 __LoSnoCo__. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface HLSSegment : NSObject

@property (nonatomic, strong) NSURL *url;
@property (nonatomic) double duration;
@property (nonatomic) NSInteger sequenceNumber;
@property (nonatomic) BOOL encrypted;
@property (nonatomic, strong) NSString *mimeType;
@property (nonatomic, strong) NSString *encryptionMethod;
@property (nonatomic, strong) NSData *encryptionKey;
@property (nonatomic, strong) NSData *iv;
@property (nonatomic) BOOL discontinuity;
@property (nonatomic, strong) NSURL *mapSectionURL;

@end
