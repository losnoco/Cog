/*
 File:		OpenURLPanel.m
 
 Originally introduced at WWDC 2004 at
 Session 214, "Programming QuickTime with Cocoa."
 Sample code is explained in detail in
 "QTKit Programming Guide" documentation.
 
 
 Copyright:	 © Copyright 2004, 2005 Apple Computer, Inc.
 All rights reserved.
 
 Disclaimer: IMPORTANT:	This Apple software is supplied to you by
 Apple Computer, Inc. ("Apple") in consideration of your agreement to the
 following terms, and your use, installation, modification or
 redistribution of this Apple software constitutes acceptance of these
 terms.	If you do not agree with these terms, please do not use,
 install, modify or redistribute this Apple software.
 
 In consideration of your agreement to abide by the following terms, and
 subject to these terms, Apple grants you a personal, non-exclusive
 license, under AppleÕs copyrights in this original Apple software (the
 "Apple Software"), to use, reproduce, modify and redistribute the Apple
 Software, with or without modifications, in source and/or binary forms;
 provided that if you redistribute the Apple Software in its entirety and
 without modifications, you must retain this notice and the following
 text and disclaimers in all such redistributions of the Apple Software.
 Neither the name, trademarks, service marks or logos of Apple Computer,
 Inc. may be used to endorse or promote products derived from the Apple
 Software without specific prior written permission from Apple.	Except
 as expressly stated in this notice, no other rights or licenses, express
 or implied, are granted by Apple herein, including but not limited to
 any patent rights that may be infringed by your derivative works or by
 other works in which the Apple Software may be incorporated.
 
 The Apple Software is provided by Apple on an "AS IS" basis.  APPLE
 MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
 THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
 FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND
 OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
 
 IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
 OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
 MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED
 AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
 STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 
 */


#import "OpenURLPanel.h"

// user defaults keys
#define kUserDefaultURLsKey @"UserDefaultURLsKey"

// maximum urls
#define kMaximumURLs		15

@implementation OpenURLPanel

static OpenURLPanel *openURLPanel = nil;

// class methods
+ (id)openURLPanel
{
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		openURLPanel = [self new];
	});
    
    return openURLPanel;
}

- (id)init
{
    if ((self = [super init]))
    {
        // init
        [self setURLArray:[NSMutableArray arrayWithCapacity:10]];
        
        // listen for app termination notifications
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(writeURLs:) name:NSApplicationWillTerminateNotification object:NSApp];
    }
    
    return self;
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [self setURLArray:nil];
}

// getters
- (NSString *)urlString
{
    NSString *urlString = nil;
    
    // get the url
    urlString = [mUrlComboBox stringValue];
    
    if (urlString == nil)
        urlString = [mUrlComboBox objectValueOfSelectedItem];
    
    if ([urlString length] == 0)
        urlString = nil;
    
    return urlString;
}

- (NSURL *)url
{
    NSURL		*url = nil;
    NSString	*urlString;
    
    // get the url
    urlString = [self urlString];
    
    if (urlString)
        url = [NSURL URLWithString:urlString];
    
    return url;
}

// setters
- (void)setURLArray:(NSMutableArray *)urlLArray
{
    mUrlArray = urlLArray;
}

// delegates
- (void)awakeFromNib
{
    NSArray *urls;
    
    // restore the previous urls
    urls = [[NSUserDefaults standardUserDefaults] objectForKey:kUserDefaultURLsKey];
    [mUrlArray addObjectsFromArray:urls];
    
    if (urls)
        [mUrlComboBox addItemsWithObjectValues:urls];
}

// notifications
- (void)writeURLs:(NSNotification *)notification
{
    NSUserDefaults *userDefaults;
    
    if ([mUrlArray count])
    {
        // init
        userDefaults = [NSUserDefaults standardUserDefaults];
        
        // write out the urls
        [userDefaults setObject:mUrlArray forKey:kUserDefaultURLsKey];
        [userDefaults synchronize];
    }
}

// actions
typedef id (*myIMP)(id, SEL, ...);

- (IBAction)doOpenURL:(id)sender
{
    NSString    *urlString;
    NSURL       *url;
    BOOL        informDelegate = YES;
    
    if ([sender tag] == NSModalResponseOK)
    {
        // validate the URL
        url = [self url];
        urlString = [self urlString];
        
        if (url)
        {
            // save the url
            if (![mUrlArray containsObject:urlString])
            {
                // save the url
                [mUrlArray addObject:urlString];
                
                // add the url to the combo box
                [mUrlComboBox addItemWithObjectValue:urlString];
                
                // remove the oldest url if the maximum has been exceeded
                if ([mUrlArray count] > kMaximumURLs)
                {
                    [mUrlArray removeObjectAtIndex:0];
                    [mUrlComboBox removeItemAtIndex:0];
                }
            }
            else
            {
                // move the url to the bottom of the list
                [mUrlArray removeObject:urlString];
                [mUrlArray addObject:urlString];
                [mUrlComboBox removeItemWithObjectValue:urlString];
                [mUrlComboBox addItemWithObjectValue:urlString];
            }
        }
        else
        {
            NSAlert *alert = [NSAlert new];
            alert.messageText = NSLocalizedString(@"InvalidURLShort", @"");
            alert.informativeText = NSLocalizedString(@"InvalidURLLong", @"");
            if (mIsSheet)
                [alert runModal];
            else
                [alert beginSheetModalForWindow:mPanel completionHandler:nil];

            informDelegate = NO;
        }
    }
    
    // inform the delegate
    if (informDelegate && mDelegate && mDidEndSelector)
    {
        NSInvocation *inv = [NSInvocation invocationWithMethodSignature:[mDelegate methodSignatureForSelector:mDidEndSelector]];
        [inv setSelector:mDidEndSelector];
        [inv setTarget:mDelegate];
        
        OpenURLPanel *pself = self;
        int tag = (int)([sender tag]);
        
        [inv setArgument:&(pself) atIndex:2];
        [inv setArgument:&(tag) atIndex:3];
        [inv setArgument:&(mContextInfo) atIndex:4];
        
        [inv invoke];
        
        [self close];
    }
}

// methods
- (void)beginSheetWithWindow:(NSWindow *)window delegate:(id)delegate didEndSelector:(SEL)didEndSelector contextInfo:(void *)contextInfo
{
    // will this run as a sheet
    mIsSheet = (window ? YES : NO);

    // save the delegate, did end selector, and context info
    mDelegate = delegate;
    mDidEndSelector = (didEndSelector);
    mContextInfo = contextInfo;
    
    NSArray *objects;

    // load the bundle (if necessary)
    if (mPanel == nil)
        [[NSBundle mainBundle] loadNibNamed:@"OpenURLPanel" owner:self topLevelObjects:&objects];

    // start the sheet (or window)
    [window beginSheet:mPanel completionHandler:nil];
}

- (void)close
{
    // close it down
    [NSApp endSheet:mPanel];
    [mPanel close];
}

@end
