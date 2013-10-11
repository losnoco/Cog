/*
	NDHotKeyEvent.m

	Created by Nathan Day on 21.06.06 under a MIT-style license. 
	Copyright (c) 2008 Nathan Day

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.
 */

#import "NDHotKeyEvent.h"
#import "NDKeyboardLayout.h"

static const NSUInteger		kNDHotKeyEventVersion = 2;

@interface NDHotKeyEvent ()
#ifdef NDMapTableClassDefined
+ (NSMapTable *)allHotKeyEvents;
#else
+ (NSHashTable *)allHotKeyEvents;
#endif
- (void)addHotKey;
- (void)removeHotKey;
- (BOOL)setCollectiveEnabled:(BOOL)aFlag;
- (BOOL)collectiveEnable;
@end

static NSString		* kArchivingKeyCodeKey = @"KeyCodeKey",
					* kArchivingKeyCharacterKey = @"KeyCharacterKey",
					* kArchivingModifierFlagsKey = @"ModifierFlagsKey",
					* kArchivingSelectorReleasedCodeKey = @"SelectorReleasedCodeKey",
					* kArchivingSelectorPressedCodeKey = @"SelectorPressedCodeKey";
const OSType		NDHotKeyDefaultSignature = 'NDHK';

static OSStatus	switchHotKey( NDHotKeyEvent * self, BOOL aFlag );

@interface NDHotKeyEvent ()
@end
/*
 * class implementation NDHotKeyEvent
 */
@implementation NDHotKeyEvent

#ifdef NDHotKeyEventThreadSafe
	#warning Thread saftey has been enabled for NDHotKeyEvent class methods
	#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
		#define	NDHotKeyEventLock @synchronized([self class]) {
		#define	NDHotKeyEventUnlock }
	#else
		static NSLock				* hotKeysLock = nil;
		#define	NDHotKeyEventLock [hotKeysLock lock]
		#define	NDHotKeyEventUnlock [hotKeysLock unlock]
	#endif
#else
	#define	NDHotKeyEventLock // lock
	#define	NDHotKeyEventUnlock // unlock
#endif

#ifdef NDMapTableClassDefined
static NSMapTable		* allHotKeyEvents = nil;
#else
static NSHashTable		* allHotKeyEvents = NULL;
#endif
static EventHandlerRef	hotKeysEventHandler = NULL;
static OSType			signature = 0;

static pascal OSErr eventHandlerCallback( EventHandlerCallRef anInHandlerCallRef, EventRef anInEvent, void * self );

#ifndef NDMapTableClassDefined
static NSUInteger hashValueHashFunction( NSHashTable * aTable, const void * aHotKeyEvent );
static BOOL isEqualHashFunction( NSHashTable * aTable, const void * aFirstHotKeyEvent, const void * aSecondHotKeyEvent);
static NSString * describeHashFunction( NSHashTable * aTable, const void * aHotKeyEvent );
#endif

static UInt32 _idForCharacterAndModifer( unichar aCharacter, NSUInteger aModFlags ) { return (NSUInteger)aCharacter | (aModFlags<<16); }

static void _getCharacterAndModiferForId( UInt32 anId, unichar *aCharacter, NSUInteger *aModFlags )
{
	*aModFlags = anId>>16;
	*aCharacter = anId&0xFFFF;
}

struct HotKeyMappingEntry
{
	UInt32				hotKeyId;
	NDHotKeyEvent		* hotKeyEvent;
};

+ (BOOL)install
{
	if( hotKeysEventHandler == NULL )
	{
		id					theHotKeyEvents = [self allHotKeyEvents];
		EventTypeSpec		theTypeSpec[] =
		{
			{ kEventClassKeyboard, kEventHotKeyPressed },
			{ kEventClassKeyboard, kEventHotKeyReleased }
		};
		
		NDHotKeyEventLock;
		if( theHotKeyEvents != nil && hotKeysEventHandler == NULL )
		{
			if( InstallEventHandler( GetEventDispatcherTarget(), NewEventHandlerUPP((EventHandlerProcPtr)eventHandlerCallback), 2, theTypeSpec, theHotKeyEvents, &hotKeysEventHandler ) != noErr )
				NSLog(@"Could not install Event handler");
		}
		NDHotKeyEventUnlock;
	}
	
	return hotKeysEventHandler != NULL;
}

+ (void)uninstall
{
	if( hotKeysEventHandler != NULL )
		RemoveEventHandler( hotKeysEventHandler );
}

+ (void)initialize
{
	[NDHotKeyEvent setVersion:kNDHotKeyEventVersion];			// the character attribute has been removed
#ifdef NDHotKeyEventThreadSafe
#if MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_4
	if( hotKeysLock == nil )
	{
		NSLock		* theInstance = [[NSLock alloc] init];

		if( !CompareAndSwap( nil, (unsigned long int)theInstance, (unsigned long int*)&hotKeysLock) )
			[theInstance release];			// did not use instance
	}
#endif
#endif
}

+ (void)setSignature:(OSType)aSignature
{
	NSAssert( signature == 0 || aSignature == signature, @"The signature used by NDHotKeyEvent can only be set once safely" );
	signature = aSignature;
}

+ (OSType)signature
{
	signature = signature ? signature : NDHotKeyDefaultSignature;
	return signature;
}

+ (BOOL)setAllEnabled:(BOOL)aFlag
{
	BOOL			theAllSucceeded = YES;
#ifdef NDMapTableClassDefined
	NSMapTable		* theAllHotKeyEvents = [NDHotKeyEvent allHotKeyEvents];
#else
	NSHashTable		* theAllHotKeyEvents = [NDHotKeyEvent allHotKeyEvents];
#endif

	/*
	 * need to install before to make sure the method 'setCollectiveEnabled:'
	 * doesn't try install since install tries to aquire the lock 'hotKeysLock'
	 */
	if( theAllHotKeyEvents && [NDHotKeyEvent install] )
	{
#ifdef NDMapTableClassDefined
		NDHotKeyEventLock;
			for( NDHotKeyEvent * theHotEvent in [theAllHotKeyEvents objectEnumerator] )
			{
				if( ![theHotEvent setCollectiveEnabled:aFlag] )
					theAllSucceeded = NO;
			}
		NDHotKeyEventUnlock;
#else
		NSHashEnumerator			theEnumerator;
		struct HotKeyMappingEntry	* theHotKeyMapEntry;
		NDHotKeyEventLock;
			theEnumerator =  NSEnumerateHashTable( theAllHotKeyEvents );

			while( (theHotKeyMapEntry = (struct HotKeyMappingEntry*)NSNextHashEnumeratorItem(&theEnumerator) ) )
			{
				if( ![theHotKeyMapEntry->hotKeyEvent setCollectiveEnabled:aFlag] )
					theAllSucceeded = NO;
			}

			NSEndHashTableEnumeration( &theEnumerator );
		NDHotKeyEventUnlock;
#endif
	}

	return theAllSucceeded;
}

+ (BOOL)isEnabledKeyCharacter:(unichar)aKeyCharacter modifierFlags:(NSUInteger)aModifierFlags
{
	return [[self findHotKeyForKeyCode:[[NDKeyboardLayout keyboardLayout] keyCodeForCharacter:aKeyCharacter numericPad:(aModifierFlags&NSNumericPadKeyMask) != 0] modifierFlags:aModifierFlags] isEnabled];
}

+ (BOOL)isEnabledKeyCode:(UInt16)aKeyCode modifierFlags:(NSUInteger)aModifierFlags
{
	return [[self findHotKeyForKeyCode:aKeyCode modifierFlags:aModifierFlags] isEnabled];
}

+ (NDHotKeyEvent *)getHotKeyForKeyCode:(UInt16)aKeyCode modifierFlags:(NSUInteger)aModifierFlags
{
	return [self getHotKeyForKeyCharacter:[[NDKeyboardLayout keyboardLayout] characterForKeyCode:aKeyCode] modifierFlags:aModifierFlags];
}

#pragma mark - finding hot key event objects
+ (NDHotKeyEvent *)getHotKeyForKeyCharacter:(unichar)aKeyCharacter modifierFlags:(NSUInteger)aModifierFlags
{
	NDHotKeyEvent		* theHotKey = nil;

	theHotKey = [self findHotKeyForKeyCharacter:aKeyCharacter modifierFlags:aModifierFlags];
	return theHotKey ? theHotKey : [self hotKeyWithKeyCharacter:aKeyCharacter modifierFlags:aModifierFlags];
}

+ (NDHotKeyEvent *)findHotKeyForKeyCode:(UInt16)aKeyCode modifierFlags:(NSUInteger)aModifierFlags
{
	return [self findHotKeyForKeyCharacter:[[NDKeyboardLayout keyboardLayout] characterForKeyCode:aKeyCode] modifierFlags:aModifierFlags];
}

+ (NDHotKeyEvent *)findHotKeyForKeyCharacter:(unichar)aKeyCharacter modifierFlags:(NSUInteger)aModifierFlags
{
	return [self findHotKeyForId:_idForCharacterAndModifer(aKeyCharacter, aModifierFlags)];
}

+ (NDHotKeyEvent *)findHotKeyForId:(UInt32)anID
{
	NDHotKeyEvent				* theResult = nil;
#ifdef NDMapTableClassDefined
	NSMapTable		* theAllHotKeyEvents = [NDHotKeyEvent allHotKeyEvents];
#else
	NSHashTable		* theAllHotKeyEvents = [NDHotKeyEvent allHotKeyEvents];
#endif
	
	if( theAllHotKeyEvents )
	{
#ifdef NDMapTableClassDefined
		NDHotKeyEventLock;
		theResult = [theAllHotKeyEvents objectForKey:[NSNumber numberWithUnsignedInt:anID]];
		NDHotKeyEventUnlock;
#else
		struct HotKeyMappingEntry		* theFoundEntry = NULL;
		struct HotKeyMappingEntry		theDummyEntry = {anID,nil};
		
		NDHotKeyEventLock;
		theFoundEntry = NSHashGet( theAllHotKeyEvents, (void*)&theDummyEntry);
		if( theFoundEntry != NULL )
			[[theFoundEntry->hotKeyEvent retain] autorelease];
		NDHotKeyEventUnlock;
		theResult = (theFoundEntry) ? theFoundEntry->hotKeyEvent : nil;
#endif
	}
	
	return theResult;
}

+ (id)hotKeyWithEvent:(NSEvent *)anEvent
{
	return [[[self alloc] initWithEvent:anEvent] autorelease];
}

+ (id)hotKeyWithEvent:(NSEvent *)anEvent target:(id)aTarget selector:(SEL)aSelector
{
	return [[[self alloc] initWithEvent:anEvent target:aTarget selector:aSelector] autorelease];
}

+ (id)hotKeyWithKeyCharacter:(unichar)aKeyCharacter modifierFlags:(NSUInteger)aModifierFlags
{
	return [[[self alloc] initWithKeyCharacter:aKeyCharacter modifierFlags:aModifierFlags target:nil selector:(SEL)0] autorelease];
}

+ (id)hotKeyWithKeyCode:(UInt16)aKeyCode modifierFlags:(NSUInteger)aModifierFlags
{
	return [[[self alloc] initWithKeyCode:aKeyCode modifierFlags:aModifierFlags target:nil selector:(SEL)0] autorelease];
}

+ (id)hotKeyWithKeyCharacter:(unichar)aKeyCharacter modifierFlags:(NSUInteger)aModifierFlags target:(id)aTarget selector:(SEL)aSelector
{
	return [[[self alloc] initWithKeyCharacter:aKeyCharacter modifierFlags:aModifierFlags target:aTarget selector:aSelector] autorelease];
}

+ (id)hotKeyWithKeyCode:(UInt16)aKeyCode modifierFlags:(NSUInteger)aModifierFlags target:(id)aTarget selector:(SEL)aSelector
{
	return [[[self alloc] initWithKeyCode:aKeyCode modifierFlags:aModifierFlags target:aTarget selector:aSelector] autorelease];
}

+ (id)hotKeyWithWithPropertyList:(id)aPropertyList
{
	return [[[self alloc] initWithPropertyList:aPropertyList] autorelease];
}

+ (NSString *)description
{
#ifdef NDMapTableClassDefined
	NSMapTable		* theAllHotKeyEvents = [NDHotKeyEvent allHotKeyEvents];
#else
	NSHashTable		* theAllHotKeyEvents = [NDHotKeyEvent allHotKeyEvents];
#endif
	NSString		* theDescription = nil;
	if( theAllHotKeyEvents )
	{
		NDHotKeyEventLock;
#ifdef NDMapTableClassDefined
		theDescription = [theAllHotKeyEvents description];
#else
		theDescription = NSStringFromHashTable(theAllHotKeyEvents);
#endif
		NDHotKeyEventUnlock;
	}
	return theDescription;
}

#pragma mark - creation and destruction
- (id)init
{
	[self release];
	NSAssert( NO, @"You can not initialize a Hot Key with the init method" );
	return nil;
}

- (id)initWithEvent:(NSEvent *)anEvent
{
	return [self initWithEvent:anEvent target:nil selector:NULL];
}

- (id)initWithEvent:(NSEvent *)anEvent target:(id)aTarget selector:(SEL)aSelector
{
	unsigned long		theModifierFlags = [anEvent modifierFlags] & (NSShiftKeyMask|NSControlKeyMask|NSAlternateKeyMask|NSCommandKeyMask);

	return [self initWithKeyCode:[anEvent keyCode]
				   modifierFlags:theModifierFlags
						  target:aTarget
						selector:aSelector];
}

- (id)initWithKeyCharacter:(unichar)aKeyCharacter modifierFlags:(NSUInteger)aModifierFlags
{
	return [self initWithKeyCode:[[NDKeyboardLayout keyboardLayout] keyCodeForCharacter:aKeyCharacter numericPad:(aModifierFlags&NSNumericPadKeyMask) != 0] modifierFlags:aModifierFlags target:nil selector:NULL];
}

- (id)initWithKeyCode:(UInt16)aKeyCode modifierFlags:(NSUInteger)aModifierFlags
{
	return [self initWithKeyCode:aKeyCode modifierFlags:aModifierFlags target:nil selector:NULL];
}

- (id)initWithKeyCode:(UInt16)aKeyCode modifierFlags:(NSUInteger)aModifierFlags target:(id)aTarget selector:(SEL)aSelector
{
	return [self initWithKeyCharacter:[[NDKeyboardLayout keyboardLayout] characterForKeyCode:aKeyCode] modifierFlags:aModifierFlags target:aTarget selector:aSelector];
}

- (id)initWithKeyCharacter:(unichar)aKeyCharacter modifierFlags:(NSUInteger)aModifierFlags target:(id)aTarget selector:(SEL)aSelector
{
	if( (self = [super init]) != nil )
	{
		keyCharacter = aKeyCharacter;
		modifierFlags = aModifierFlags;
		target = aTarget;
		selectorReleased = aSelector;
		currentEventType = NDHotKeyNoEvent;
		isEnabled.collective = YES;
		[self addHotKey];
	}
	else
		[self release], self = nil;

	return self;
}

- (id)initWithCoder:(NSCoder *)aDecoder
{
	if( (self = [super init]) != nil)
	{
		if( [aDecoder allowsKeyedCoding] )
		{
			NSNumber	* theValue = [aDecoder decodeObjectForKey:kArchivingKeyCharacterKey];
			if( theValue == nil )
			{
				theValue = [aDecoder decodeObjectForKey:kArchivingKeyCodeKey];
				keyCharacter = [[NDKeyboardLayout keyboardLayout] characterForKeyCode:[theValue unsignedShortValue]];
			}
			else
				keyCharacter = [theValue unsignedShortValue];
			modifierFlags = [[aDecoder decodeObjectForKey:kArchivingModifierFlagsKey] unsignedIntegerValue];
			
			selectorReleased = NSSelectorFromString( [aDecoder decodeObjectForKey:kArchivingSelectorReleasedCodeKey] );
			selectorPressed = NSSelectorFromString( [aDecoder decodeObjectForKey:kArchivingSelectorPressedCodeKey] );
		}
		else
		{
			if( [aDecoder versionForClassName:@"NDHotKeyNoEvent"] == 1 )
			{
				unsigned short		theKeyCode;
				[aDecoder decodeValueOfObjCType:@encode(UInt16) at:&theKeyCode];
				keyCharacter = [[NDKeyboardLayout keyboardLayout] characterForKeyCode:theKeyCode];
			}
			else
				[aDecoder decodeValueOfObjCType:@encode(unichar) at:&keyCharacter];
			[aDecoder decodeValueOfObjCType:@encode(NSUInteger) at:&modifierFlags];

			selectorReleased = NSSelectorFromString( [aDecoder decodeObject] );
			selectorPressed = NSSelectorFromString( [aDecoder decodeObject] );
		}

		[self addHotKey];
	}

	return self;
}

- (void)encodeWithCoder:(NSCoder *)anEncoder
{
	if( [anEncoder allowsKeyedCoding] )
	{
		[anEncoder encodeObject:[NSNumber numberWithUnsignedShort:keyCharacter] forKey:kArchivingKeyCharacterKey];
		[anEncoder encodeObject:[NSNumber numberWithUnsignedInteger:modifierFlags] forKey:kArchivingModifierFlagsKey];

		[anEncoder encodeObject:NSStringFromSelector( selectorReleased ) forKey:kArchivingSelectorReleasedCodeKey];
		[anEncoder encodeObject:NSStringFromSelector( selectorPressed ) forKey:kArchivingSelectorPressedCodeKey];
	}
	else
	{
		[anEncoder encodeValueOfObjCType:@encode(unichar) at:&keyCharacter];
		[anEncoder encodeValueOfObjCType:@encode(NSUInteger) at:&modifierFlags];

		[anEncoder encodeObject:NSStringFromSelector( selectorReleased )];
		[anEncoder encodeObject:NSStringFromSelector( selectorPressed )];
	}
}

- (id)initWithPropertyList:(id)aPropertyList
{
	if( aPropertyList )
	{
		NSNumber	* theKeyCode,
					* theModiferFlag;

		theKeyCode = [aPropertyList objectForKey:kArchivingKeyCodeKey];
		theModiferFlag = [aPropertyList objectForKey:kArchivingModifierFlagsKey];

		if( (self = [self initWithKeyCode:[theKeyCode unsignedShortValue] modifierFlags:[theModiferFlag unsignedIntValue]]) != nil )
		{
			selectorPressed = NSSelectorFromString([aPropertyList objectForKey:kArchivingSelectorPressedCodeKey]);
			selectorReleased = NSSelectorFromString([aPropertyList objectForKey:kArchivingSelectorReleasedCodeKey]);
		}
	}
	else
		[self release], self = nil;

	return self;
}

- (id)propertyList
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithUnsignedShort:[self keyCode]], kArchivingKeyCodeKey,
		[NSNumber numberWithUnsignedInt:[self modifierFlags]], kArchivingModifierFlagsKey,
		NSStringFromSelector( selectorPressed ), kArchivingSelectorPressedCodeKey,
		NSStringFromSelector( selectorReleased ), kArchivingSelectorReleasedCodeKey,
		nil];
}

- (oneway void)release
{
	/*
	 *	We need to remove the hot key from the hash table before it's retain count reaches zero
	 */
	if( [self retainCount] == 1 )
	{
#ifdef NDMapTableClassDefined
		NSMapTable		* theAllHotKeyEvents = [NDHotKeyEvent allHotKeyEvents];
#else
		NSHashTable		* theAllHotKeyEvents = [NDHotKeyEvent allHotKeyEvents];
#endif
		if( theAllHotKeyEvents )
		{
#ifndef NDMapTableClassDefined
			struct HotKeyMappingEntry		theDummyEntry = {[self hotKeyId],nil};
#endif
			NDHotKeyEventLock;
				if( [self retainCount] == 1 )		// check again because it might have changed
				{
					switchHotKey( self, NO );
#ifdef NDMapTableClassDefined
					[theAllHotKeyEvents removeObjectForKey:[NSNumber numberWithUnsignedInt:[self hotKeyId]]];
#else
					id		theHotKeyEvent = NSHashGet( theAllHotKeyEvents, (void*)&theDummyEntry );
					if( theHotKeyEvent )
						NSHashRemove( theAllHotKeyEvents, theHotKeyEvent );
#endif
				}
			NDHotKeyEventUnlock;
		}
	}
	[super release];
}

#if 0
- (void)dealloc
{
	if( reference )
	{
		if( UnregisterEventHotKey( reference ) != noErr )	// in lock from release
			NSLog( @"Failed to unregister hot key %@", self );
	}
	[super dealloc];
}
#endif

- (BOOL)setEnabled:(BOOL)aFlag
{
	BOOL		theResult = YES;

	if( [NDHotKeyEvent install] )
	{
		/*
		 * if individual and collective YES then currently ON, otherwise currently off
		 */
		NDHotKeyEventLock;
			if( aFlag == YES && isEnabled.collective == YES  && isEnabled.individual == NO )
				theResult = (switchHotKey( self, YES ) == noErr);
			else if( aFlag == NO && isEnabled.collective == YES  && isEnabled.individual == YES )
				theResult = (switchHotKey( self, NO ) == noErr);
		NDHotKeyEventUnlock;

		if( theResult )
			isEnabled.individual = aFlag;
		else
			NSLog(@"%s failed ", aFlag ? "enable" : "disable" );
	}
	else
		theResult = NO;

	return theResult;
}

- (void)setIsEnabled:(BOOL)aFlag { [self setEnabled:aFlag]; }

- (BOOL)isEnabled { return isEnabled.individual && isEnabled.collective; }
- (id)target { return target; }
- (SEL)selector { return selectorReleased; }
- (SEL)selectorReleased { return selectorReleased; }
- (SEL)selectorPressed { return selectorPressed; }
- (int)currentEventType { return currentEventType; }				// (NDHotKeyNoEvent | NDHotKeyPressedEvent | NDHotKeyReleasedEvent)
- (BOOL)setTarget:(id)aTarget selector:(SEL)aSelector { return [self setTarget:aTarget selectorReleased:aSelector selectorPressed:(SEL)0]; }

#ifdef NS_BLOCKS_AVAILABLE
- (BOOL)setBlock:(void(^)(NDHotKeyEvent*))aBlock { return [self setReleasedBlock:aBlock pressedBlock:nil]; }
#endif

- (BOOL)setTarget:(id)aTarget selectorReleased:(SEL)aSelectorReleased selectorPressed:(SEL)aSelectorPressed
{
	BOOL	theResult = NO;
	[self setEnabled:NO];
	if( target != nil && target != aTarget )
	{
		if( ![target respondsToSelector:@selector(targetWillChangeToObject:forHotKeyEvent:)] || [target targetWillChangeToObject:aTarget forHotKeyEvent:self] )
		{
			target = aTarget;
			theResult = YES;
		}
	}
	else
	{
		target = aTarget;
		theResult = YES;
	}

	selectorReleased = aSelectorReleased;
	selectorPressed = aSelectorPressed;

#ifdef NS_BLOCKS_AVAILABLE
	[releasedBlock release], releasedBlock = nil;
	[pressedBlock release], pressedBlock = nil;
#endif

	return theResult;		// was change succesful
}

#ifdef NS_BLOCKS_AVAILABLE
- (BOOL)setReleasedBlock:(void(^)(NDHotKeyEvent*))aReleasedBlock pressedBlock:(void(^)(NDHotKeyEvent*))aPressedBlock
{
	BOOL	theResult = NO;
	[self setEnabled:NO];
	if( ![target respondsToSelector:@selector(targetWillChangeToObject:forHotKeyEvent:)] || [target targetWillChangeToObject:nil forHotKeyEvent:self] )
	{
		if( releasedBlock != aReleasedBlock )
			[releasedBlock release], releasedBlock = [aReleasedBlock copy];

		if( pressedBlock != aPressedBlock )
			[pressedBlock release], pressedBlock = [aPressedBlock copy];

		selectorReleased = (SEL)0;
		selectorPressed = (SEL)0;
		theResult = YES;
	}
	
	return theResult;		// was change succesful
}
#endif

- (void)performHotKeyReleased
{
	NSAssert( target != nil || releasedBlock != nil, @"Release hot key fired without target or release block" );

	currentEventType = NDHotKeyReleasedEvent;
	if( selectorReleased )
	{
		if([target respondsToSelector:selectorReleased])
			[target performSelector:selectorReleased withObject:self];
		else if( [target respondsToSelector:@selector(makeObjectsPerformSelector:withObject:)] )
			[target makeObjectsPerformSelector:selectorReleased withObject:self];
	}
#ifdef NS_BLOCKS_AVAILABLE
	else if( releasedBlock )
		releasedBlock(self);
#endif
	currentEventType = NDHotKeyNoEvent;
}

- (void)performHotKeyPressed
{
	NSAssert( target != nil || pressedBlock != nil, @"Release hot key fired without target or pressed block" );

	currentEventType = NDHotKeyPressedEvent;
	if( selectorPressed )
	{
		if([target respondsToSelector:selectorPressed])
			[target performSelector:selectorPressed withObject:self];
		else if( [target respondsToSelector:@selector(makeObjectsPerformSelector:withObject:)] )
			[target makeObjectsPerformSelector:selectorPressed withObject:self];
	}
#ifdef NS_BLOCKS_AVAILABLE
	else if( pressedBlock )
		pressedBlock(self);
#endif

	currentEventType = NDHotKeyNoEvent;
}

- (unichar)keyCharacter { return keyCharacter; }
- (BOOL)keyPad { return keyPad; }
- (UInt16)keyCode { return [[NDKeyboardLayout keyboardLayout] keyCodeForCharacter:self.keyCharacter numericPad:self.keyPad]; }
- (NSUInteger)modifierFlags { return modifierFlags; }
- (UInt32)hotKeyId { return _idForCharacterAndModifer( self.keyCharacter, self.modifierFlags ); }
- (NSString *)stringValue { return [[NDKeyboardLayout keyboardLayout] stringForKeyCode:[self keyCode] modifierFlags:[self modifierFlags]]; }

- (BOOL)isEqual:(id)anObject
{
	return [super isEqual:anObject] || ([anObject isKindOfClass:[self class]] == YES && [self keyCode] == [(NDHotKeyEvent*)anObject keyCode] && [self modifierFlags] == [anObject modifierFlags]);
}

- (NSUInteger)hash { return (NSUInteger)self.keyCharacter | (self.modifierFlags<<16); }

- (NSString *)description
{
	return [NSString stringWithFormat:@"{\n\tKey Combination: %@,\n\tEnabled: %s\n\tKey Press Selector: %@\n\tKey Release Selector: %@\n}\n",
					[self stringValue],
					[self isEnabled] ? "yes" : "no",
					NSStringFromSelector([self selectorPressed]),
					NSStringFromSelector([self selectorReleased])];
}

pascal OSErr eventHandlerCallback( EventHandlerCallRef anInHandlerCallRef, EventRef anInEvent, void * anInUserData )
{
//	NSHashTable			* allHotKeyEvents = (NSHashTable *)anInUserData;
	EventHotKeyID		theHotKeyID;
	OSStatus			theError;

	NSCAssert( GetEventClass( anInEvent ) == kEventClassKeyboard, @"Got event that is not a hot key event" );

	theError = GetEventParameter( anInEvent, kEventParamDirectObject, typeEventHotKeyID, NULL, sizeof(EventHotKeyID), NULL, &theHotKeyID );

	if( theError == noErr )
	{
		NDHotKeyEvent		* theHotKeyEvent;
		UInt32				theEventKind;
		
		NSCAssert( [NDHotKeyEvent signature] == theHotKeyID.signature, @"Got hot key event with wrong signature" );

		theHotKeyEvent = [NDHotKeyEvent findHotKeyForId:theHotKeyID.id];

		theEventKind = GetEventKind( anInEvent );
		if( kEventHotKeyPressed == theEventKind )
			[theHotKeyEvent performHotKeyPressed];
		else if( kEventHotKeyReleased == theEventKind )
			[theHotKeyEvent performHotKeyReleased];
	}

	return theError;
}

#ifndef NDMapTableClassDefined

NSUInteger hashValueHashFunction( NSHashTable * aTable, const void * aHotKeyEntry )
{
	struct HotKeyMappingEntry	* theHotKeyEntry = (struct HotKeyMappingEntry*)aHotKeyEntry;
	return  theHotKeyEntry->hotKeyId;
}

BOOL isEqualHashFunction( NSHashTable * aTable, const void * aFirstHotKeyEntry, const void * aSecondHotKeyEntry)
{
	struct HotKeyMappingEntry		* theFirst = (struct HotKeyMappingEntry*)aFirstHotKeyEntry,
									* theSecond = (struct HotKeyMappingEntry*)aSecondHotKeyEntry;
	return theFirst->hotKeyId == theSecond->hotKeyId;
}

NSString * describeHashFunction( NSHashTable * aTable, const void * aHotKeyEntry )
{
	NDHotKeyEvent		* theHotKey;

	theHotKey = ((struct HotKeyMappingEntry*)aHotKeyEntry)->hotKeyEvent;
	return [theHotKey description];
}
#endif

#pragma mark Private methods

#ifdef NDMapTableClassDefined
+ (NSMapTable *)allHotKeyEvents
{
	if( allHotKeyEvents == NULL )
	{
		NDHotKeyEventLock;
		if( allHotKeyEvents == NULL )
			allHotKeyEvents = [[NSMapTable alloc] initWithKeyOptions:NSMapTableStrongMemory valueOptions:NSMapTableZeroingWeakMemory capacity:0];
		NDHotKeyEventUnlock;
	}
	return allHotKeyEvents;
}
#else

+ (NSHashTable *)allHotKeyEvents
{
	if( allHotKeyEvents == NULL )
	{
		NSHashTableCallBacks		theHashCallBacks;

		theHashCallBacks.hash = hashValueHashFunction;
		theHashCallBacks.isEqual = isEqualHashFunction;
		theHashCallBacks.retain = NULL;
		theHashCallBacks.release = NULL;
		theHashCallBacks.describe = describeHashFunction;

		NDHotKeyEventLock;
			if( allHotKeyEvents == NULL )
				allHotKeyEvents = NSCreateHashTableWithZone( theHashCallBacks, 0, NULL);
		NDHotKeyEventUnlock;
	}

	return allHotKeyEvents;
}
#endif

- (void)addHotKey
{
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardLayoutSelectedKeyboardInputSourceChangedNotification:) name:NDKeyboardLayoutSelectedKeyboardInputSourceChangedNotification object:nil];
#ifdef NDMapTableClassDefined
	NSMapTable			* theAllHotKeyEvents = [NDHotKeyEvent allHotKeyEvents];
#else
	NSHashTable			* theAllHotKeyEvents = [NDHotKeyEvent allHotKeyEvents];
#endif
	if( theAllHotKeyEvents )
	{
		NDHotKeyEventLock;
#ifdef NDMapTableClassDefined
			[theAllHotKeyEvents setObject:self forKey:[NSNumber numberWithUnsignedInt:[self hotKeyId]]];
#else
			struct HotKeyMappingEntry	* theEntry = (struct HotKeyMappingEntry *)malloc(sizeof(struct HotKeyMappingEntry));
			theEntry->hotKeyId = [self hotKeyId];
			theEntry->hotKeyEvent = self;

			NSParameterAssert( NSHashInsertIfAbsent( theAllHotKeyEvents, (void*)theEntry ) == NULL );
#endif
		NDHotKeyEventUnlock;
	}
}

- (void)keyboardLayoutSelectedKeyboardInputSourceChangedNotification:(NSNotification*)aNotification
{
	if( self.isEnabled )		// if enable re-eable using new keyCode
		switchHotKey( self, YES );
}

- (void)removeHotKey
{
	[self setEnabled:NO];

#ifdef NDMapTableClassDefined
	NSMapTable			* theAllHotKeyEvents = [NDHotKeyEvent allHotKeyEvents];
#else
	NSHashTable			* theAllHotKeyEvents = [NDHotKeyEvent allHotKeyEvents];
#endif

	if( theAllHotKeyEvents )
	{
#ifndef NDMapTableClassDefined
		struct HotKeyMappingEntry	theDummyEntry = {[self hotKeyId],nil};
		struct HotKeyMappingEntry	* theEntry = NULL;
#endif
		NDHotKeyEventLock;
#ifdef NDMapTableClassDefined
			[theAllHotKeyEvents removeObjectForKey:[NSNumber numberWithUnsignedInt:[self hotKeyId]]];
#else
			theEntry = (struct HotKeyMappingEntry*)NSHashGet( theAllHotKeyEvents, (void*)&theDummyEntry);
			if( theEntry )
			{
				NSParameterAssert( theEntry->hotKeyEvent == self );
				NSHashRemove( theAllHotKeyEvents, theEntry );
			}
#endif
		NDHotKeyEventUnlock;
	}
}

- (BOOL)setCollectiveEnabled:(BOOL)aFlag
{
	BOOL		theResult = YES;
	
	if( [NDHotKeyEvent install] )
	{
		/*
		 * if individual and collective YES then currently ON, otherwise currently off
		 */
		NDHotKeyEventLock;
			if( aFlag == YES && isEnabled.collective == NO  && isEnabled.individual == YES )
				theResult = (switchHotKey( self, YES ) == noErr);
			else if( aFlag == NO && isEnabled.collective == YES  && isEnabled.individual == YES )
				theResult = (switchHotKey( self, NO ) == noErr);
		NDHotKeyEventUnlock;

		if( theResult )
			isEnabled.collective = aFlag;
		else
			NSLog(@"%s failed", aFlag ? "enable" : "disable" );
	}
	else
		theResult = NO;

	return theResult;
}

- (BOOL)collectiveEnable { return isEnabled.collective; }

static OSStatus switchHotKey( NDHotKeyEvent * self, BOOL aFlag )
{
	OSStatus		theError = noErr;
	if( aFlag )
	{
		EventHotKeyID 		theHotKeyID;

		if( self->reference )
			theError = UnregisterEventHotKey( self->reference );
		if( theError == noErr )
		{
			theHotKeyID.signature = [NDHotKeyEvent signature];
			theHotKeyID.id = [self hotKeyId];

			NSCAssert( theHotKeyID.signature, @"HotKeyEvent signature has not been set yet" );
			NSCParameterAssert(sizeof(unsigned long) >= sizeof(id) );

			theError = RegisterEventHotKey( self.keyCode, NDCarbonModifierFlagsForCocoaModifierFlags(self->modifierFlags), theHotKeyID, GetEventDispatcherTarget(), 0, &self->reference );
		}
	}
	else
	{
		theError = UnregisterEventHotKey( self->reference );
		self->reference = 0;
	}

	return theError;
}

#pragma mark - Deprecated Methods

+ (NDHotKeyEvent *)getHotKeyForKeyCode:(UInt16)aKeyCode character:(unichar)aChar modifierFlags:(NSUInteger)aModifierFlags
{
	return [self getHotKeyForKeyCode:aKeyCode modifierFlags:aModifierFlags];
}

/*
 * +hotKeyWithKeyCode:character:modifierFlags:
 */
+ (id)hotKeyWithKeyCode:(UInt16)aKeyCode character:(unichar)aChar modifierFlags:(NSUInteger)aModifierFlags
{
	return [self hotKeyWithKeyCode:aKeyCode modifierFlags:aModifierFlags target:nil selector:NULL];
}

/*
 * +hotKeyWithKeyCode:character:modifierFlags:target:selector:
 */
+ (id)hotKeyWithKeyCode:(UInt16)aKeyCode character:(unichar)aChar modifierFlags:(NSUInteger)aModifierFlags target:(id)aTarget selector:(SEL)aSelector
{
	return [[[self alloc] initWithKeyCode:aKeyCode modifierFlags:aModifierFlags target:aTarget selector:aSelector] autorelease];
}

/*
 * -initWithKeyCode:character:modifierFlags:
 */
- (id)initWithKeyCode:(UInt16)aKeyCode character:(unichar)aChar modifierFlags:(NSUInteger)aModifierFlags
{
	return [self initWithKeyCode:aKeyCode modifierFlags:aModifierFlags target:nil selector:NULL];
}

/*
 * -initWithKeyCode:character:modifierFlags:target:selector:
 */
- (id)initWithKeyCode:(UInt16)aKeyCode character:(unichar)aChar modifierFlags:(NSUInteger)aModifierFlags target:(id)aTarget selector:(SEL)aSelector
{
	return [self initWithKeyCode:aKeyCode modifierFlags:aModifierFlags target:aTarget selector:aSelector];
}

@end
