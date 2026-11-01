//
//  TimeField.h
//  Cog
//
//  Created by Vincent Spader on 2/22/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

/// Centres the digits in the field's bezel. The field is a small control
/// with a larger font, which makes it taller than its bezel, and its own
/// layout left the digits low.
@interface TimeFieldCell : NSTextFieldCell
@end

@interface TimeField : NSTextField

@property(nonatomic) NSInteger currentTime;
@property(nonatomic) NSInteger duration;

@end
