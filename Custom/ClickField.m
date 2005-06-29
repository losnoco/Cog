#import "ClickField.h"

@implementation ClickField

- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent {
	return YES;
}

- (void)mouseDown:(NSEvent *)theEvent
{
	if([theEvent type] == NSLeftMouseDown)
	{
		[soundController toggleShowTimeRemaining:self];
	}
}

@end
