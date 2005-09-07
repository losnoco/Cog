//
//  InputController.m
//  Cog
//
//  Created by Zaphod Beeblebrox on 8/2/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import "InputController.h"


@implementation InputController

- (void)play
{
	[SoundFile open:filename];
}

@end
