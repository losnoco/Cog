//
//  Shuffle.m
//  Cog
//
//  Created by Zaphod Beeblebrox on 1/14/06.
//  Copyright 2006 __MyCompanyName__. All rights reserved.
//

#import "Shuffle.h"


@implementation Shuffle

int sum(int n)
{
	return (n*n+n)/2;
}

int reverse_sum(int n)
{
	return (int)(ceil((-1.0 + sqrt(1.0 + 8.0*n))/2.0));
}

int randint(int low, int high)
{
	return (random()%high)+low;
}

+ (NSMutableArray *)shuffleList:(NSArray *)l
{
	NSMutableArray *a = [l mutableCopy];
	NSMutableArray *b = [[NSMutableArray alloc] init];
	
	while([a count] > 0)
	{
		int t, r, p;
		
		t = sum([a count]);
		r = randint(1, t);
		p = reverse_sum(r) - 1;
		printf("%i, %i, %i, %i\n", [a count], t, r, p);
		
		[b insertObject:[a objectAtIndex:p] atIndex:0];
		[a removeObjectAtIndex:p];
	}		
	
	[a release];
	
	return [b autorelease];
}

@end
