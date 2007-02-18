//
//  KFTypeSelectTableView.h
//  KFTypeSelectTableView v1.0.4
//
//  Keyboard navigation enabled table view.  Suitable for 
//  class posing as well as normal use.
//
//  All delegate methods are optional, except you need to implement typeSelectTableView:stringValueForTableColumn:row:
//  if you're using bindings to supply the table view with data.
//  
//  ------------------------------------------------------------------------
//  Copyright (c) 2005, Ken Ferry All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are
//  met:
//
//  (1) Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.
//
//  (2) Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//      
//  (3) Neither Ken Ferry's name nor the names of other contributors
//      may be used to endorse or promote products derived from this software
//      without specific prior written permission.
//
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
//  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
//  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
//  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
//  OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
//  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//  ------------------------------------------------------------------------
// 

#import <Cocoa/Cocoa.h>

#pragma mark constants

typedef enum KFTypeSelectMatchAlgorithm {
    KFSubstringMatchAlgorithm = 0,
    KFPrefixMatchAlgorithm = 1
} KFTypeSelectMatchAlgorithm;

@interface KFTypeSelectTableView : NSTableView

#pragma mark action methods

// these beep if the operation cannot be performed
- (void)findNext:(id)sender;
- (void)findPrevious:(id)sender;

#pragma mark accessors
// KVO-compliant
- (NSString *)pattern;

// a tableview with no match algorithm set uses defaultMatchAlgorithm
// defaultMatchAlgorithm defaults to KFPrefixMatchAlgorithm
+ (KFTypeSelectMatchAlgorithm)defaultMatchAlgorithm;
+ (void)setDefaultMatchAlgorithm:(KFTypeSelectMatchAlgorithm)algorithm;

- (KFTypeSelectMatchAlgorithm)matchAlgorithm;
- (void)setMatchAlgorithm:(KFTypeSelectMatchAlgorithm)algorithm;

// defaults to NO
- (BOOL)searchWraps;
- (void)setSearchWraps:(BOOL)flag;

// supply a set of identifiers to limit columns searched for match.
// Only columns with identifiers in the provided set are searched.
// nil identifiers means search all columns.  defaults to nil.
- (NSSet *)searchColumnIdentifiers;
- (void)setSearchColumnIdentifiers:(NSSet *)identifiers;

@end

@interface NSObject (KFTypeSelectTableViewDelegate) 

#pragma mark configuration methods 

// Implement this method if the table uses bindings for data.
// Use something like
//     return [[[arrayController arrangedObjects] objectAtIndex:row] valueForKey:[column identifier]];
// Could also use it to supply string representations for non-string data, or to search only part of visible text.
- (NSString *)typeSelectTableView:(id)tableView stringValueForTableColumn:(NSTableColumn *)column row:(int)row;

// defaults to YES
- (BOOL)typeSelectTableViewSearchTopToBottom:(id)tableView; 

 // defaults to first or last row, depending on direction of search
- (int)typeSelectTableViewInitialSearchRow:(id)tableView;   

// A hook for cases (like mail plugin) where there's no good place to configure the table.
// Will be called before type-select is used with any particular delegate.
- (void)configureTypeSelectTableView:(id)tableView;

#pragma mark reporting methods
// pattern of @"" indicates no search, anything else means a search is in progress
// userInfo dictionary has @"oldPattern" key
// this notification is sent
//    when a search begins or is modified
//    when a search is cancelled
//    x seconds after a search either succeeds or fails, where x is a timeout period
- (void)typeSelectTableViewPatternDidChange:(NSNotification *)aNotification; 
- (void)typeSelectTableView:(id)tableView didFindMatch:(NSString *)match range:(NSRange)matchedRange forPattern:(NSString *)pattern;
- (void)typeSelectTableView:(id)tableView didFailToFindMatchForPattern:(NSString *)pattern; // fallback is a beep if delegate does not implement

@end

#pragma mark notifications
// delegate automatically receives this notification.  See delegate method above.
extern NSString *KFTypeSelectTableViewPattenDidChangeNotification;
