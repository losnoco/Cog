//
//  HLSMemorySource.h
//  HLS
//
//  Created by Claude on 2026-05-05.
//  Lightly edited by Christopher Snowhill on 2026-05-05
//  Copyright 2026 __LoSnoCo__. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "Plugin.h"

@interface HLSMemorySource : NSObject <CogSource>

@property (nonatomic, strong) NSMutableData *data;
@property (nonatomic) NSUInteger position;
@property (nonatomic, strong) NSURL *url;
@property (nonatomic, strong) NSString *mimeType;
@property (nonatomic) NSUInteger positionOffset;
@property (readonly) NSUInteger segmentsBuffered;

- (id)initWithURL:(NSURL *)url mimeType:(NSString *)mimeType;
- (void)appendData:(NSData *)newData;
- (void)resetPosition;

@end
