//
//  CogSearchField.m
//  Cog
//
//  Created by Christopher Snowhill on 9/30/25.
//

#import "CogSearchField.h"

NS_ASSUME_NONNULL_BEGIN

@implementation CogSearchField

- (void)cancelOperation:(nullable id)sender {
	[mainWindow makeFirstResponder:nil];
	[mainWindow resignFirstResponder];
}

@end

NS_ASSUME_NONNULL_END
