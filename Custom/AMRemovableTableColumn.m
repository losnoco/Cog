//
//  AMRemovableTableColumn.m
//  HebX
//
//  Created by Andreas on 28.08.05.
//  Copyright 2005 Andreas Mayer. All rights reserved.
//

#import "AMRemovableTableColumn.h"

@interface AMRemovableColumnsTableView (Private)
- (void)am_hideTableColumn:(NSTableColumn *)column;
- (void)am_showTableColumn:(NSTableColumn *)column;
@end


@implementation AMRemovableTableColumn

static BOOL AMRemovableTableColumn_frameworkDoesSupportHiddenColumns = NO;

+ (void)initialize
{
	// should the framework support isHidden/setHidden:, use the build-in methods
	AMRemovableTableColumn_frameworkDoesSupportHiddenColumns = [NSTableColumn instancesRespondToSelector:@selector(setHidden:)];
}


- (AMRemovableColumnsTableView *)mainTableView
{
	return mainTableView; 
}

- (void)setMainTableView:(AMRemovableColumnsTableView *)newMainTableView
{
	// do not retain
	mainTableView = newMainTableView;
}


- (BOOL)isHidden
{
	if (AMRemovableTableColumn_frameworkDoesSupportHiddenColumns) {
		return [(id)super isHidden]; // cast to id to avoid compiler warning
	} else {
		return ([self tableView] != mainTableView);
	}
}

- (void)setHidden:(BOOL)flag
{
	if (AMRemovableTableColumn_frameworkDoesSupportHiddenColumns) {
		[(id)super setHidden:flag]; // cast to id to avoid compiler warning
	} else {
		if (flag) {
			[(AMRemovableColumnsTableView *)[self tableView] am_hideTableColumn:self];
		} else {
			[mainTableView am_showTableColumn:self];
		}
	}
}


@end
