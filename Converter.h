//
//  Converter.h
//  Cog
//
//  Created by Zaphod Beeblebrox on 8/2/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface Converter : NSObject {
	SoundController *soundController;
}

- (void)setup;
- (void)cleanUp;

- (void)process;
- (int)convert:(void *)dest amount:(int)amount;

@end
