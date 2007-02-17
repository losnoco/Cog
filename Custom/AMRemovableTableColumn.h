//
//  AMRemovableTableColumn.h
//  HebX
//
//  Created by Andreas on 28.08.05.
//  Copyright 2005 Andreas Mayer. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "AMRemovableColumnsTableView.h"


@interface AMRemovableTableColumn : NSTableColumn {
	IBOutlet AMRemovableColumnsTableView *mainTableView;
}

- (AMRemovableColumnsTableView *)mainTableView;
- (void)setMainTableView:(AMRemovableColumnsTableView *)newMainTableView;

- (BOOL)isHidden;
- (void)setHidden:(BOOL)flag;


@end
