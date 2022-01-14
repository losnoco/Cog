//
//  RefillNode.m
//  Cog
//
//  Created by Christopher Snowhill on 1/13/22.
//  Copyright 2022 __LoSnoCo__. All rights reserved.
//

#import "Plugin.h"
#import "RefillNode.h"

#import "Logging.h"

@implementation RefillNode

- (id)initWithController:(id)c previous:(id)p {
    self = [super initWithController:c previous:p];

    return self;
}


- (void)writeData:(float *)data floatCount:(size_t)count
{
    [self writeData:data amount:(int)(count * sizeof(float))];
}


- (void)dealloc
{
	DLog(@"Refill Node dealloc");
}

@end
