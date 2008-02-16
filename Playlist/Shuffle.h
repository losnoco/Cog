//
//  Shuffle.h
//  Cog
//
//  Created by Vincent Spader on 1/14/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface Shuffle : NSObject {

}

+ (void)initialize;
+ (NSMutableArray *)shuffleList:(NSArray *)l;

@end
