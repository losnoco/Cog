#import "ClickField.h"

@implementation ClickField

- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent {
	return YES;
}

- (void)mouseDown:(NSEvent *)theEvent
{
	NSLog(@"MOUSING DOWN");
	if([theEvent type] == NSLeftMouseDown)
	{
//		NSLog(@"SENDING ACTION: %@", [self action]);
		[self sendAction:[self action] to:[self target]];
	}
}

@end
