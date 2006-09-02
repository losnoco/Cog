//
//  FileOutlineView.m
//  BindTest
//
//  Created by Zaphod Beeblebrox on 8/20/06.
//  Copyright 2006 __MyCompanyName__. All rights reserved.
//

#import "FileOutlineView.h"
#import "FileIconCell.h"

@implementation FileOutlineView

- (void) awakeFromNib
{
	NSLog(@"FILE OUTLINE VIEW");
	
	NSEnumerator *e = [[self tableColumns] objectEnumerator];
	id c;
	while ((c = [e nextObject]))
	{
//		id headerCell = [[ImageTextCell alloc] init];
		id dataCell = [[FileIconCell alloc] init];
		
		[dataCell setLineBreakMode:NSLineBreakByTruncatingTail];
//		[c setHeaderCell: headerCell];
		[c setDataCell: dataCell];
	}
}
		
@end
